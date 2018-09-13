#include "ctlspriv.h"
#include "toolbar.h"
#include "help.h" // Help IDs

#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    (void)SendMessage((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))

#define SPACESTRLEN 20

#define FLAG_NODEL  0x8000
#define FLAG_HIDDEN 0x4000
#define FLAG_SEP    0x2000
#define FLAG_ALLFLAGS   (FLAG_NODEL|FLAG_HIDDEN|FLAG_SEP)

typedef struct {        /* instance data for toolbar edit dialog */
    HWND hDlg;          /* dialog hwnd */
    PTBSTATE ptb;       // current toolbar state
    int iPos;           /* position to insert into */
} ADJUSTDLGDATA, *LPADJUSTDLGDATA;


int g_dyButtonHack = 0;     // to pass on before WM_INITDIALOG

LPTSTR TB_StrForButton(PTBSTATE ptb, LPTBBUTTONDATA pTBButton);

int GetPrevButton(PTBSTATE ptb, int iPos)
{
    /* This means to delete the preceding space
     */
    for (--iPos; ; --iPos)
    {
        if (iPos < 0)
            break;

        if (!(ptb->Buttons[iPos].fsState & TBSTATE_HIDDEN))
            break;;
    }

    return(iPos);
}

BOOL GetAdjustInfo(PTBSTATE ptb, int iItem, LPTBBUTTONDATA ptbButton, LPTSTR lpString, int cbString)
{
    TBNOTIFY tbn;
    tbn.pszText = lpString;
    tbn.cchText = cbString;
    tbn.iItem = iItem;

    if (lpString)
        *lpString = 0;

    if ((BOOL)CCSendNotify(&ptb->ci, TBN_GETBUTTONINFO, &tbn.hdr))
    {
        TBInputStruct(ptb, ptbButton, &tbn.tbButton);
        return TRUE;
    }
    return FALSE;
}

LRESULT SendItemNotify(PTBSTATE ptb, int iItem, int code)
{
    TBNOTIFY tbn = {0};
    tbn.iItem = iItem;

    switch (code) {

    case TBN_QUERYDELETE:
    case TBN_QUERYINSERT:
        // The following is to provide the parent app with information
        // about the button that information is being requested for...
        // Otherwise it's really awful trying to have control over
        // certain aspects of toolbar customization... [t-mkim]
        // IE4.0's toolbar wants this information.
        //      Should ONLY be done for TBN_QUERY* notifications BECAUSE
        //      this can be either a zero-based index _or_ Command ID depending
        //      on the particular notification code.
        if (iItem < ptb->iNumButtons)
            CopyMemory (&tbn.tbButton, &ptb->Buttons[iItem], sizeof (TBBUTTON));
        break;

    case TBN_DROPDOWN:
        TB_GetItemRect(ptb, PositionFromID(ptb, iItem), &tbn.rcButton);
        break;
    }

    // default return from SendNotify is false
    return BOOLFROMPTR(CCSendNotify(&ptb->ci, code, &tbn.hdr));
}

#define SendCmdNotify(ptb, code)   CCSendNotify(&ptb->ci, code, NULL)


// this is used to deal with the case where the ptb structure is re-alloced
// after a TBInsertButtons()

PTBSTATE FixPTB(HWND hwnd)
{
    PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwnd, 0);

    if (ptb->hdlgCust)
    {
        LPADJUSTDLGDATA lpad = (LPADJUSTDLGDATA)GetWindowPtr(ptb->hdlgCust, DWLP_USER);
#ifdef DEBUG
        if (lpad->ptb != ptb)
            DebugMsg(DM_TRACE, TEXT("Fixing busted ptb pointer"));
#endif
        lpad->ptb = ptb;
    }
    return ptb;
}


void MoveButton(PTBSTATE ptb, int nSource)
{
    int nDest;
    RECT rc;
    HCURSOR hCursor;
    MSG32 msg32;

    /* You can't move separators like this
     */
    if (nSource < 0)
        return;

    // Make sure it is all right to "delete" the selected button
    if (!SendItemNotify(ptb, nSource, TBN_QUERYDELETE))
        return;

    hCursor = SetCursor(LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_MOVEBUTTON)));
    SetCapture(ptb->ci.hwnd);

    // Get the dimension of the window.
    GetClientRect(ptb->ci.hwnd, &rc);
    for ( ; ; )
    {
        while (!PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE))
            ;

        if (GetCapture() != ptb->ci.hwnd)
            goto AbortMove;

        // See if the application wants to process the message...
        if (CallMsgFilter32(&msg32, MSGF_COMMCTRL_TOOLBARCUST, TRUE) != 0)
            continue;


        switch (msg32.message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
#ifdef KEYBOARDCUES
            //notify of navigation key usage
            CCNotifyNavigationKeyUsage(&(ptb->ci), UISF_HIDEFOCUS);
#endif
            break;

        case WM_LBUTTONUP:
            RelayToToolTips(ptb->hwndToolTips, ptb->ci.hwnd, msg32.message, msg32.wParam, msg32.lParam);
            if ((GET_Y_LPARAM(msg32.lParam) > (short)(rc.bottom+ptb->iButWidth)) ||
                (GET_X_LPARAM(msg32.lParam) > (short)(rc.right+ptb->iButWidth)) ||
                (GET_Y_LPARAM(msg32.lParam) < -ptb->iButWidth) ||
                (GET_X_LPARAM(msg32.lParam) < -ptb->iButWidth))

            {
                /* If the button was dragged off the toolbar, delete it.
                 */
DeleteSrcButton:
                DeleteButton(ptb, nSource);
                SendCmdNotify(ptb, TBN_TOOLBARCHANGE);
                TBInvalidateItemRects(ptb);
            }
            else
            {
                TBBUTTONDATA tbbAdd;

                /* Add half a button to X so that it looks like it is centered
                 * over the target button, iff we have a horizontal layout.
                 * Add half a button to Y otherwise.
                 */
                if (rc.right!=ptb->iButWidth)
                    nDest = TBHitTest(ptb,
                                      GET_X_LPARAM(msg32.lParam) + ptb->iButWidth / 2,
                                      GET_Y_LPARAM(msg32.lParam));
                else
                    nDest = TBHitTest(ptb,
                                      GET_X_LPARAM(msg32.lParam),
                                      GET_Y_LPARAM(msg32.lParam) + ptb->iButHeight / 2);

                if (nDest < 0)
                    nDest = -1 - nDest;

                if (nDest>0 &&
                    (ptb->Buttons[nDest-1].fsState & TBSTATE_WRAP) &&
                    GET_X_LPARAM(msg32.lParam)>ptb->iButWidth &&
                    SendItemNotify(ptb, --nDest, TBN_QUERYINSERT))
                {
                    tbbAdd = ptb->Buttons[nSource];
                    DeleteButton(ptb, nSource);
                    if (nDest>nSource)
                        --nDest;

                    /* Insert before spaces, but after buttons. */
                    if (!(ptb->Buttons[nDest].fsStyle & TBSTYLE_SEP))
                        nDest++;

                    goto InsertSrcButton;
                }
                else if (nDest == nSource)
                {
                    /* This means to delete the preceding space, or to move a
                    button to the previous row.
                    */
                    nSource = GetPrevButton(ptb, nSource);
                    if (nSource < 0)
                        goto AbortMove;

                    // If the preceding item is a space with no ID, and
                    // the app says it's OK, then delete it.
                    if ((ptb->Buttons[nSource].fsStyle & TBSTYLE_SEP)
                        && !ptb->Buttons[nSource].idCommand
                        && SendItemNotify(ptb, nSource, TBN_QUERYDELETE))
                        goto DeleteSrcButton;
                }
                else if (nDest == nSource+1)
                {
                    // This means to add a preceding space
                    --nDest;
                    if (SendItemNotify(ptb, nDest, TBN_QUERYINSERT))
                    {
                        tbbAdd.DUMMYUNION_MEMBER(iBitmap) = 0;
                        tbbAdd.idCommand = 0;
                        tbbAdd.iString = -1;
                        tbbAdd.fsState = 0;
                        tbbAdd.fsStyle = TBSTYLE_SEP;
                        goto InsertSrcButton;
                    }
                }
                else if (SendItemNotify(ptb, nDest, TBN_QUERYINSERT))
                {
                    HWND hwndT;
                    TBBUTTON tbbAddExt;

                    /* This is a normal move operation
                     */
                    tbbAdd = ptb->Buttons[nSource];

                    ptb->Buttons[nSource].iString = -1;
                    DeleteButton(ptb, nSource);
                    if (nDest > nSource)
                        --nDest;
InsertSrcButton:
                    hwndT = ptb->ci.hwnd;

                    TBOutputStruct(ptb, &tbbAdd, &tbbAddExt);
                    TBInsertButtons(ptb, nDest, 1, &tbbAddExt, TRUE);

                    ptb = FixPTB(hwndT);

                    SendCmdNotify(ptb, TBN_TOOLBARCHANGE);
                    TBInvalidateItemRects(ptb);
                }
                else
                {
AbortMove:
                    ;
                }
            }
            goto AllDone;

        case WM_RBUTTONDOWN:
            goto AbortMove;

        default:
            TranslateMessage32(&msg32, TRUE);
            DispatchMessage32(&msg32, TRUE);
            break;
        }
    }
AllDone:

    SetCursor(hCursor);
    CCReleaseCapture(&ptb->ci);
}


#define GNI_HIGH    0x0001
#define GNI_LOW     0x0002

int GetNearestInsert(PTBSTATE ptb, int iPos, int iNumButtons, UINT uFlags)
{
    int i;
    BOOL bKeepTrying;

    // Find the nearest index where we can actually insert items
    for (i = iPos; ; ++i, --iPos)
    {
        bKeepTrying = FALSE;

        // Notice we favor going high if both flags are set
        if ((uFlags & GNI_HIGH) && i <= iNumButtons)
        {
            bKeepTrying = TRUE;

            if (SendItemNotify(ptb, i, TBN_QUERYINSERT))
                return i;
        }

        if ((uFlags & GNI_LOW) && iPos >= 0)
        {
            bKeepTrying = TRUE;

            if (SendItemNotify(ptb, iPos, TBN_QUERYINSERT))
                return iPos;
        }

        if (!bKeepTrying)
            return -1;   // There was no place to add buttons
    }
}


BOOL InitAdjustDlg(HWND hDlg, LPADJUSTDLGDATA lpad)
{
    HDC hDC;
    HFONT hFont;
    HWND hwndCurrent, hwndNew;
    LPTBBUTTONDATA ptbButton;
    int i, iPos, nItem, nWid, nMaxWid;
    TBBUTTONDATA tbAdjust;
    TCHAR szDesc[128];
    NMTBCUSTOMIZEDLG nm;
    TCHAR szSeparator[MAX_PATH];

    szSeparator[0] = 0;
    LocalizedLoadString(IDS_SPACE, szSeparator, ARRAYSIZE(szSeparator));

    lpad->hDlg = hDlg;
    lpad->ptb->hdlgCust = hDlg;

    /* Determine the item nearest the desired item that will allow
     * insertion.
     */
    iPos = GetNearestInsert(lpad->ptb, lpad->iPos, lpad->ptb->iNumButtons,
                            GNI_HIGH | GNI_LOW);
    if (iPos < 0)
    /* No item allowed insertion, so leave the dialog */
    {
        return(FALSE);
    }

    /* Reset the lists of used and available items.
     */
    hwndCurrent = GetDlgItem(hDlg, IDC_CURRENT);
    SendMessage(hwndCurrent, LB_RESETCONTENT, 0, 0L);

    hwndNew = GetDlgItem(hDlg, IDC_BUTTONLIST);
    SendMessage(hwndNew, LB_RESETCONTENT, 0, 0L);

    nm.hDlg = hDlg;
    if (CCSendNotify(&lpad->ptb->ci, TBN_INITCUSTOMIZE, &nm.hdr) == TBNRF_HIDEHELP) {
        ShowWindow(GetDlgItem(hDlg, IDC_APPHELP), SW_HIDE);
    }

    for (i=0, ptbButton = lpad->ptb->Buttons; i < lpad->ptb->iNumButtons; ++i, ++ptbButton)
    {
        UINT uFlags;
        int iBitmap;
        LPTSTR pszStr = NULL;

        uFlags = 0;

        // Non-deletable and hidden items show up grayed.

        if (!SendItemNotify(lpad->ptb, i, TBN_QUERYDELETE))
        {
            uFlags |= FLAG_NODEL;
        }
        if (ptbButton->fsState & TBSTATE_HIDDEN)
        {
            uFlags |= FLAG_HIDDEN;
        }

        /* Separators have no bitmaps (even ones with IDs).  Only set
         * the separator flag if there is no ID (it is a "real"
         * separator rather than an owner item).
         */
        if (ptbButton->fsStyle&TBSTYLE_SEP)
        {
            if (!(ptbButton->idCommand))
            {
                uFlags |= FLAG_SEP;
            }
            iBitmap = -1;

            pszStr = szSeparator;
        }
        else
        {
            iBitmap = ptbButton->DUMMYUNION_MEMBER(iBitmap);
            // this specifies an imagelist.
            // pack this into the loword of the ibitmap.
            // this causes a restriction of max 16 imagelists, and 4096 images in any imagelist
            iBitmap = LOWORD(iBitmap) | (HIWORD(iBitmap) << 12);

            /* Add the item and the data
             * Note: A negative number in the LOWORD indicates no bitmap;
             * otherwise it is the bitmap index.
             */
            pszStr = TB_StrForButton(lpad->ptb, ptbButton);
        }

        if ((int)SendMessage(hwndCurrent, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)(pszStr ? pszStr : (LPTSTR)c_szNULL)) != i)
        {
            return(FALSE);
        }
        SendMessage(hwndCurrent, LB_SETITEMDATA, i, MAKELPARAM(iBitmap, uFlags));
    }

    /* Add a dummy "nodel" space at the end so things can be inserted at the end.
     */
    if ((int)SendMessage(hwndCurrent, LB_ADDSTRING, 0,(LPARAM)(LPTSTR)szSeparator) == i)
    {
        SendMessage(hwndCurrent, LB_SETITEMDATA, i, MAKELPARAM(-1, FLAG_NODEL|FLAG_SEP));
    }

    /* Now add a space at the beginning of the "new" list.
     */
        if (SendMessage(hwndNew, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)szSeparator) == LB_ERR)
            return(FALSE);
            
        SendMessage(hwndNew, LB_SETITEMDATA, 0, MAKELPARAM(-1, FLAG_SEP));

    /* We need this to determine the widest (in pixels) item string.
     */
    hDC = GetDC(hwndCurrent);
    hFont = (HFONT)(INT_PTR)SendMessage(hwndCurrent, WM_GETFONT, 0, 0L);
    if (hFont)
    {
        hFont = SelectObject(hDC, hFont);
    }
    nMaxWid = 0;

    for (i=0; ; ++i)
    {
        // Get the info about the i'th item from the app.
        if (!GetAdjustInfo(lpad->ptb, i, &tbAdjust, szDesc, ARRAYSIZE(szDesc)))
            break;
        
        if (!szDesc[0]) {
            LPTSTR psz = TB_StrForButton(lpad->ptb, &tbAdjust);
            if (psz) {
                lstrcpyn(szDesc, psz, ARRAYSIZE(szDesc));
            }
        }

        /* Don't show separators that don't have commands
         */
        if (!(tbAdjust.fsStyle & TBSTYLE_SEP) || tbAdjust.idCommand)
        {
            
            /* Get the maximum width of a string.
             */
            MGetTextExtent(hDC, szDesc, lstrlen(szDesc), &nWid, NULL);

            if (nMaxWid < nWid)
            {
                nMaxWid = nWid;
            }

            nItem = PositionFromID(lpad->ptb, tbAdjust.idCommand);
            if (nItem < 0)
            /* If the item is not on the toolbar already */
            {
#ifdef UNIX
                if (!lstrcmp(szDesc, TEXT("Folders")) || !lstrcmp(szDesc, TEXT("Edit")))
                    continue;
#endif

                /* Don't show hidden buttons
                 */
                if (!(tbAdjust.fsState & TBSTATE_HIDDEN))
                {
                    nItem = (int)SendMessage(hwndNew, LB_ADDSTRING, 0,
                                             (LPARAM)(LPTSTR)szDesc);
                    if (nItem != LB_ERR)
                    {
                        
                        if (tbAdjust.fsStyle & TBSTYLE_SEP)
                            SendMessage(hwndNew, LB_SETITEMDATA, nItem,
                                        MAKELPARAM(-1, i));
                        else {
                            int iBitmap = tbAdjust.DUMMYUNION_MEMBER(iBitmap);
                            iBitmap = LOWORD(iBitmap) | (HIWORD(iBitmap) << 12);
                            SendMessage(hwndNew, LB_SETITEMDATA, nItem,
                                        MAKELPARAM(iBitmap, i));
                        }
                    }
                }
            }
            else
            /* The item is on the toolbar already */
            {
                /* Preserve the flags and bitmap.
                 */
                DWORD dwTemp = (DWORD)SendMessage(hwndCurrent, LB_GETITEMDATA, nItem, 0L);

                if (szDesc[0]) {
                    SendMessage(hwndCurrent, LB_DELETESTRING, nItem, 0L);

                    if ((int)SendMessage(hwndCurrent, LB_INSERTSTRING, nItem,
                                         (LPARAM)(LPTSTR)szDesc) != nItem)
                    {
                        ReleaseDC(hwndCurrent, hDC);
                        return(FALSE);
                    }
                }
                SendMessage(hwndCurrent, LB_SETITEMDATA, nItem,
                    MAKELPARAM(LOWORD(dwTemp), HIWORD(dwTemp)|i));
            }
        }
    }

    if (hFont)
    {
        SelectObject(hDC, hFont);
    }
    ReleaseDC(hwndCurrent, hDC);

    /* Add on some extra and set the extents for both lists.
     */
    nMaxWid += lpad->ptb->iButWidth + 2 + 1;
    SendMessage(hwndNew, LB_SETHORIZONTALEXTENT, nMaxWid, 0L);
    SendMessage(hwndCurrent, LB_SETHORIZONTALEXTENT, nMaxWid, 0L);

    /* Set the sels and return.
     */
    SendMessage(hwndNew, LB_SETCURSEL, 0, 0L);
    SendMessage(hwndCurrent, LB_SETCURSEL, iPos, 0L);
    SEND_WM_COMMAND(hDlg, IDC_CURRENT, hwndCurrent, LBN_SELCHANGE);

    return(TRUE);
}


#define IsSeparator(x) (HIWORD(x) & FLAG_SEP)

void PaintAdjustLine(PTBSTATE ptb, DRAWITEMSTRUCT *lpdis)
{
    HDC hdc = lpdis->hDC;
    HWND hwndList = lpdis->hwndItem;
    PTSTR pszText;
    RECT rc = lpdis->rcItem;
    int nBitmap, nLen, nItem = lpdis->itemID;
    COLORREF oldBkColor, oldTextColor;
    BOOL bSelected, bHasFocus;
    int wHeight;
    int x;


    if (lpdis->CtlID != IDC_BUTTONLIST && lpdis->CtlID != IDC_CURRENT)
        return;

    nBitmap = LOWORD(lpdis->itemData);
    // unpack the nBitmap.  we stored the imagelist spec in the hi char of loword
    if (nBitmap != 0xFFFF)
        nBitmap = (nBitmap & 0x0FFF) | ((nBitmap & 0xF000) << 4);

    nLen = (int)SendMessage(hwndList, LB_GETTEXTLEN, nItem, 0L);
    if (nLen < 0)
        return;

    pszText = (PTSTR)LocalAlloc(LPTR, (nLen+1)*sizeof(TCHAR));
    if (!pszText)
        return;

    // This needs to work for separators also or ActiveAccessibility
    // won't work.
    SendMessage(hwndList, LB_GETTEXT, nItem, (LPARAM)(LPTSTR)pszText);
    if (lpdis->itemAction != ODA_FOCUS)
    {
        COLORREF clr;
        TCHAR szSample[2];

        /* We don't care about focus if the item is not selected.
        */
        bSelected = lpdis->itemState & ODS_SELECTED;
        bHasFocus = bSelected && (GetFocus() == hwndList);

        if (HIWORD(lpdis->itemData) & (FLAG_NODEL | FLAG_HIDDEN))
            clr = g_clrGrayText;
        else if (bHasFocus)
            clr = g_clrHighlightText;
        else
            clr = g_clrWindowText;

        oldTextColor = SetTextColor(hdc, clr);
        oldBkColor = SetBkColor(hdc, bHasFocus ? g_clrHighlight : g_clrWindow);

        szSample[0] = TEXT('W');
        szSample[1] = TEXT('\0');

        MGetTextExtent(hdc, szSample, 1, NULL, &wHeight);

        x = rc.left + 2;
        x += (ptb->ci.style & TBSTYLE_FLAT) ? (ptb->iDxBitmap + g_cxEdge) : ptb->iButWidth;
        ExtTextOut(hdc, x,
                   (rc.top + rc.bottom-wHeight) / 2,
                   ETO_CLIPPED | ETO_OPAQUE, &rc, pszText, nLen, NULL);

        /* We really care about the bitmap value here; this is not just an
        * indicator for the separator.
        */
        if (nBitmap >= 0)
        {
            TBBUTTONDATA tbbAdd = {0};
            TBDRAWITEM tbdraw = {0};

            tbbAdd.DUMMYUNION_MEMBER(iBitmap) = nBitmap;
            tbbAdd.iString = -1;
            tbbAdd.fsStyle = TBSTYLE_BUTTON;
            tbbAdd.fsState = (BYTE)((HIWORD(lpdis->itemData) & FLAG_HIDDEN) ? 0 : TBSTATE_ENABLED);

            InitTBDrawItem(&tbdraw, ptb, &tbbAdd, tbbAdd.fsState, 0, 0, 0);

            if (ptb->ci.style & TBSTYLE_FLAT)
                DrawFace(hdc, rc.left + 1, rc.top + 1, 0, 0, 0, 0, &tbdraw);
            else
                DrawButton(hdc, rc.left + 1, rc.top + 1, ptb, &tbbAdd, TRUE);
            ReleaseMonoDC(ptb);
        }

        SetBkColor(hdc, oldBkColor);
        SetTextColor(hdc, oldTextColor);

        /* Frame the item if it is selected but does not have the focus.
        */
        if (bSelected && !bHasFocus)
        {
            nLen = rc.left + (int)SendMessage(hwndList,
            LB_GETHORIZONTALEXTENT, 0, 0L);
            if (rc.right < nLen)
                rc.right = nLen;

            FrameRect(hdc, &rc, g_hbrHighlight);
        }
    }

    if ((lpdis->itemAction == ODA_FOCUS || (lpdis->itemState & ODS_FOCUS))
#ifdef KEYBOARDCUES
        && !(CCGetUIState(&(ptb->ci)) & UISF_HIDEFOCUS)
#endif
        )
        DrawFocusRect(hdc, &rc); 

    LocalFree((HLOCAL)pszText);
}


void LBMoveButton(LPADJUSTDLGDATA lpad, UINT wIDSrc, int iPosSrc,
      UINT wIDDst, int iPosDst, int iSelOffset)
{
    HWND hwndSrc, hwndDst;
    DWORD dwDataSrc;
    PTSTR pStr;
    TBBUTTONDATA tbAdjust = {0};
    TBBUTTON tbbAddExt;
    int iTopDst;
    TCHAR szDesc[128];

    hwndSrc = GetDlgItem(lpad->hDlg, wIDSrc);
    hwndDst = GetDlgItem(lpad->hDlg, wIDDst);

    // Make sure we can delete the source and insert at the dest
    //
    dwDataSrc = (DWORD)SendMessage(hwndSrc, LB_GETITEMDATA, iPosSrc, 0L);
    if (iPosSrc < 0 || (HIWORD(dwDataSrc) & FLAG_NODEL))
        return;
    if (wIDDst == IDC_CURRENT && 
        !SendItemNotify(lpad->ptb, iPosDst, TBN_QUERYINSERT))
        return;

    // Get the string for the source
    //
    pStr = (PTSTR)LocalAlloc(LPTR,
        ((int)(SendMessage(hwndSrc, LB_GETTEXTLEN, iPosSrc, 0L))+1)*sizeof(TCHAR));
    if (!pStr)
        return;
    SendMessage(hwndSrc, LB_GETTEXT, iPosSrc, (LPARAM)(LPTSTR)pStr);

    SendMessage(hwndSrc, WM_SETREDRAW, 0, 0L);
    SendMessage(hwndDst, WM_SETREDRAW, 0, 0L);
    iTopDst = (int)SendMessage(hwndDst, LB_GETTOPINDEX, 0, 0L);

    // If we are inserting into the available button list, we need to determine
    // the insertion point
    //
    if (wIDDst == IDC_BUTTONLIST)
    {
        // Insert this back in the available list if this is not a space or a
        // hidden button.
        //
        if (HIWORD(dwDataSrc)&(FLAG_SEP|FLAG_HIDDEN))
        {
            iPosDst = 0;
            goto DelTheSrc;
        }
        else
        {
            UINT uCmdSrc = HIWORD(dwDataSrc) & ~(FLAG_ALLFLAGS);

            // This just does a linear search for where to put the
            // item.  Slow, but this only happens when the user clicks
            // the "Remove" button.
            //
            iPosDst = 1;
            
            for ( ; ; ++iPosDst)
            {
                // Notice that this will break out when iPosDst is
                // past the number of items, since -1 will be returned
                //
                if ((UINT)HIWORD(SendMessage(hwndDst, LB_GETITEMDATA,
                    iPosDst, 0L)) >= uCmdSrc)
                break;
            }
        }
    }
    else if (iPosDst < 0)
        goto CleanUp;

    // Attempt to insert the new string
    //
    if ((int)SendMessage(hwndDst, LB_INSERTSTRING, iPosDst, (LPARAM)(LPTSTR)pStr)
      == iPosDst)
    {
        // Attempt to sync up the actual toolbar.
        //
        if (wIDDst == IDC_CURRENT)
        {
            HWND hwndT;

            if (IsSeparator(dwDataSrc))
            {
                // Make up a dummy lpInfo if this is a space
                //
                tbAdjust.DUMMYUNION_MEMBER(iBitmap) = 0;
                tbAdjust.idCommand = 0;
                tbAdjust.fsState = 0;
                tbAdjust.fsStyle = TBSTYLE_SEP;
            }
            else
            {
                // Call back to client to get the source button info
                //
                int iCmdSrc = HIWORD(dwDataSrc) & ~FLAG_ALLFLAGS;
                if (!GetAdjustInfo(lpad->ptb, iCmdSrc, &tbAdjust, szDesc, ARRAYSIZE(szDesc)))
                    goto DelTheDst;
            }

            hwndT = lpad->ptb->ci.hwnd;

            TBOutputStruct(lpad->ptb, &tbAdjust, &tbbAddExt);
            if (!TBInsertButtons(lpad->ptb, iPosDst, 1, &tbbAddExt, TRUE))
            {
DelTheDst:
                SendMessage(hwndDst, LB_DELETESTRING, iPosDst, 0L);
                goto CleanUp;
            }
            else
            {
                lpad->ptb = FixPTB(hwndT);
            }

            if (wIDSrc == IDC_CURRENT && iPosSrc >= iPosDst)
                ++iPosSrc;
        }

        SendMessage(hwndDst, LB_SETITEMDATA, iPosDst, dwDataSrc);

DelTheSrc:
        // Don't delete the "Separator" in the new list
        //
        if ((wIDSrc != IDC_BUTTONLIST) || (iPosSrc != 0))
        {
            SendMessage(hwndSrc, LB_DELETESTRING, iPosSrc, 0L);
            if (wIDSrc == wIDDst)
            {
                if (iPosSrc < iPosDst)
                    --iPosDst;
                if (iPosSrc < iTopDst)
                    --iTopDst;
            }
        }

        // Delete the corresponding button
        //
        if (wIDSrc == IDC_CURRENT)
            DeleteButton(lpad->ptb, iPosSrc);

        // Only set the src index if the two windows are different
        //
        if (wIDSrc != wIDDst)
        {
            if (iPosSrc >= SendMessage(hwndSrc, LB_GETCOUNT, 0, 0L))
            {
                // HACKHACK: workaround for funkdified listbox scrolling behavior.
                // Select the first item (to force scroll back to top of list),
                // then select the item we really want selected.
                SendMessage(hwndSrc, LB_SETCURSEL, 0, 0L);
            }

            if (SendMessage(hwndSrc, LB_SETCURSEL, iPosSrc, 0L) == LB_ERR)
                SendMessage(hwndSrc, LB_SETCURSEL, iPosSrc-1, 0L);
            SEND_WM_COMMAND(lpad->hDlg, wIDSrc, hwndSrc, LBN_SELCHANGE);
        }

        // Send the final SELCHANGE message after everything else is done
        //
        SendMessage(hwndDst, LB_SETCURSEL, iPosDst+iSelOffset, 0L);
        SEND_WM_COMMAND(lpad->hDlg, wIDDst, hwndDst, LBN_SELCHANGE);
    }

CleanUp:

    LocalFree((HLOCAL)pStr);

    if (wIDSrc == wIDDst)
    {
        SendMessage(hwndDst, LB_SETTOPINDEX, iTopDst, 0L);
        //make sure that the selected item is still  visible
        SendMessage(hwndDst, LB_SETCURSEL, (int)SendMessage(hwndDst, LB_GETCURSEL, 0, 0L), 0);
    }
    SendMessage(hwndSrc, WM_SETREDRAW, 1, 0L);
    SendMessage(hwndDst, WM_SETREDRAW, 1, 0L);

    InvalidateRect(hwndDst, NULL, TRUE);

    SendCmdNotify(lpad->ptb, TBN_TOOLBARCHANGE);
}


void SafeEnableWindow(HWND hDlg, UINT wID, HWND hwndDef, BOOL bEnable)
{
    HWND hwndEnable;

    hwndEnable = GetDlgItem(hDlg, wID);

    if (!bEnable && GetFocus()==hwndEnable)
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndDef, 1L);
    EnableWindow(hwndEnable, bEnable);
}

int InsertIndex(LPADJUSTDLGDATA lpad, POINT pt, BOOL bDragging)
{
    HWND hwndCurrent = GetDlgItem(lpad->hDlg, IDC_CURRENT);
    int nItem = LBItemFromPt(hwndCurrent, pt, bDragging);
    if (nItem >= 0)
    {
        if (!SendItemNotify(lpad->ptb, nItem, TBN_QUERYINSERT))
            nItem = -1;
    }

    DrawInsert(lpad->hDlg, hwndCurrent, bDragging ? nItem : -1);

    return(nItem);
}


BOOL IsInButtonList(HWND hDlg, POINT pt)
{
    ScreenToClient(hDlg, &pt);

    return(ChildWindowFromPoint(hDlg, pt) == GetDlgItem(hDlg, IDC_BUTTONLIST));
}


BOOL HandleDragMsg(LPADJUSTDLGDATA lpad, HWND hDlg, WPARAM wID, LPDRAGLISTINFO lpns)
{
    switch (wID)
    {
    case IDC_CURRENT:
        switch (lpns->uNotification)
        {
        case DL_BEGINDRAG:
            {
                int nItem = (int)SendMessage(lpns->hWnd, LB_GETCURSEL, 0, 0L);
                if (HIWORD(SendMessage(lpns->hWnd, LB_GETITEMDATA, nItem, 0L)) & FLAG_NODEL)
                    return SetDlgMsgResult(hDlg, WM_COMMAND, FALSE);
                return SetDlgMsgResult(hDlg, WM_COMMAND, TRUE);
            }
            
        case DL_DRAGGING:
            {
                int nDropIndex;

DraggingSomething:
                nDropIndex = InsertIndex(lpad, lpns->ptCursor, TRUE);
                if (nDropIndex>=0 || IsInButtonList(hDlg, lpns->ptCursor))
                {
                    SetCursor(LoadCursor(HINST_THISDLL,
                        MAKEINTRESOURCE(IDC_MOVEBUTTON)));
                    return SetDlgMsgResult(hDlg, WM_COMMAND, 0);
                }
                return SetDlgMsgResult(hDlg, WM_COMMAND, DL_STOPCURSOR);
            }
            
        case DL_DROPPED:
            {
                int nDropIndex, nSrcIndex;
                
                nDropIndex = InsertIndex(lpad, lpns->ptCursor, FALSE);
                nSrcIndex = (int)SendMessage(lpns->hWnd, LB_GETCURSEL, 0, 0L);
                
                if (nDropIndex >= 0)
                {
                    if ((UINT)(nDropIndex-nSrcIndex) > 1)
                        LBMoveButton(lpad, IDC_CURRENT, nSrcIndex,
                        IDC_CURRENT, nDropIndex, 0);
                }
                else if (IsInButtonList(hDlg, lpns->ptCursor))
                {
                    LBMoveButton(lpad, IDC_CURRENT, nSrcIndex, IDC_BUTTONLIST, 0, 0);
                }
                break;
            }
            
        case DL_CANCELDRAG:
CancelDrag:
            /* This erases the insert icon if it exists.
             */
            InsertIndex(lpad, lpns->ptCursor, FALSE);
            break;
            
        default:
            break;
        }
        break;
        
        case IDC_BUTTONLIST:
            switch (lpns->uNotification)
            {
            case DL_BEGINDRAG:
                return SetDlgMsgResult(hDlg, WM_COMMAND, TRUE);
                
            case DL_DRAGGING:
                goto DraggingSomething;
                
            case DL_DROPPED:
                {
                    int nDropIndex;
                    
                    nDropIndex = InsertIndex(lpad, lpns->ptCursor, FALSE);
                    if (nDropIndex >= 0)
                        LBMoveButton(lpad, IDC_BUTTONLIST,
                            (int)SendMessage(lpns->hWnd,LB_GETCURSEL,0,0L),
                            IDC_CURRENT, nDropIndex, 0);
                    break;
                }
                
            case DL_CANCELDRAG:
                goto CancelDrag;
                
            default:
                break;
            }
            break;
            
            default:
                break;
    }
    
    return(0);
}


#ifndef WINNT
#pragma data_seg(DATASEG_READONLY)
#endif
const static DWORD aAdjustHelpIDs[] = {  // Context Help IDs
    IDC_RESET,       IDH_COMCTL_RESET,
    IDC_APPHELP,     IDH_HELP,
    IDC_MOVEUP,      IDH_COMCTL_MOVEUP,
    IDC_MOVEDOWN,    IDH_COMCTL_MOVEDOWN,
    IDC_BUTTONLIST,  IDH_COMCTL_AVAIL_BUTTONS,
    IDOK,            IDH_COMCTL_ADD,
    IDC_REMOVE,      IDH_COMCTL_REMOVE,
    IDC_CURRENT,     IDH_COMCTL_BUTTON_LIST,
    IDCANCEL,        IDH_COMCTL_CLOSE,
    0, 0
};
#ifndef WINNT
#pragma data_seg()
#endif

BOOL_PTR CALLBACK AdjustDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPADJUSTDLGDATA lpad = (LPADJUSTDLGDATA)GetWindowPtr(hDlg, DWLP_USER);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);  /* LPADJUSTDLGDATA pointer */
        if (!InitAdjustDlg(hDlg, (LPADJUSTDLGDATA)lParam))
            EndDialog(hDlg, FALSE);
        
        ShowWindow(hDlg, SW_SHOW);
        UpdateWindow(hDlg);
        SetFocus(GetDlgItem(hDlg, IDC_CURRENT));
        
        MakeDragList(GetDlgItem(hDlg, IDC_CURRENT));
        MakeDragList(GetDlgItem(hDlg, IDC_BUTTONLIST));
        
        return FALSE;
        
    case WM_MEASUREITEM:
#define lpmis ((MEASUREITEMSTRUCT *)lParam)
        
        if (lpmis->CtlID == IDC_BUTTONLIST || lpmis->CtlID == IDC_CURRENT)
        {
            int nHeight;
            HWND hwndList = GetDlgItem(hDlg, lpmis->CtlID);
            HDC hDC = GetDC(hwndList);
            TCHAR szSample[2];
            
            szSample[0] = TEXT('W');
            szSample[1] = TEXT('\0');
            
            MGetTextExtent(hDC, szSample, 1, NULL, &nHeight);
            
            // note, we use this lame hack because we get WM_MEASUREITEMS
            // before our WM_INITDIALOG where we get the lpad setup
            
            if (nHeight < g_dyButtonHack + 2)
                nHeight = g_dyButtonHack + 2;
            
            lpmis->itemHeight = nHeight;
            ReleaseDC(hwndList, hDC);
        }
        break;
        
    case WM_DRAWITEM:
        PaintAdjustLine(lpad->ptb, (DRAWITEMSTRUCT *)lParam);
        break;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aAdjustHelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID) aAdjustHelpIDs);
        break;
        
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_APPHELP:
            SendCmdNotify(lpad->ptb, TBN_CUSTHELP);
            break;
            
        case IDOK:
            {
                int iPos, nItem;
                
                nItem = (int)SendDlgItemMessage(hDlg, IDC_BUTTONLIST,
                    LB_GETCURSEL, 0, 0L);
                
                iPos = (int)SendDlgItemMessage(hDlg, IDC_CURRENT,
                    LB_GETCURSEL, 0, 0L);
                
                if (iPos == -1)
                    iPos = 0;
                
                LBMoveButton(lpad, IDC_BUTTONLIST, nItem, IDC_CURRENT, iPos, 1);
                break;
            }
            
        case IDC_BUTTONLIST:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_DBLCLK:
                SendMessage(hDlg, WM_COMMAND, IDOK, 0L);
                break;
                
            case LBN_SETFOCUS:
            case LBN_KILLFOCUS:
                {
                    RECT rc;
                    
                    if (SendMessage(GET_WM_COMMAND_HWND(wParam, lParam), LB_GETITEMRECT,
                        (int)SendMessage(GET_WM_COMMAND_HWND(wParam, lParam), LB_GETCURSEL,
                        0, 0L), (LPARAM)(LPRECT)&rc) != LB_ERR)
                        InvalidateRect(GET_WM_COMMAND_HWND(wParam, lParam), &rc, FALSE);
                }
                
            default:
                break;
            }
            break;
            
        case IDC_CURRENT:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_SELCHANGE:
                {
                    BOOL bDelOK;
                    HWND hwndList = GET_WM_COMMAND_HWND(wParam, lParam);
                    int iPos = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
                    
                    SafeEnableWindow(hDlg, IDOK, hwndList, BOOLFROMPTR(SendItemNotify(lpad->ptb, iPos, TBN_QUERYINSERT)));
                    
                    bDelOK = !(HIWORD(SendMessage(hwndList, LB_GETITEMDATA, iPos, 0L)) & FLAG_NODEL);
                    
                    SafeEnableWindow(hDlg, IDC_REMOVE, hwndList, bDelOK);
                    
                    SafeEnableWindow(hDlg, IDC_MOVEUP, hwndList, bDelOK &&
                        GetNearestInsert(lpad->ptb, iPos - 1, 0, GNI_LOW) >= 0);
                    
                    SafeEnableWindow(hDlg, IDC_MOVEDOWN, hwndList, bDelOK &&
                        GetNearestInsert(lpad->ptb, iPos + 2,
                        lpad->ptb->iNumButtons, GNI_HIGH) >=0 );
                    break;
                }
                
            case LBN_DBLCLK:
                SendMessage(hDlg, WM_COMMAND, IDC_REMOVE, 0L);
                break;
                
            case LBN_SETFOCUS:
            case LBN_KILLFOCUS:
                {
                    RECT rc;

                    if (SendMessage(GET_WM_COMMAND_HWND(wParam, lParam), LB_GETITEMRECT,
                        (int)SendMessage(GET_WM_COMMAND_HWND(wParam, lParam), LB_GETCURSEL,
                        0, 0L), (LPARAM)(LPRECT)&rc) != LB_ERR)
                        InvalidateRect(GET_WM_COMMAND_HWND(wParam, lParam), &rc, FALSE);
                }
                
            default:
                break;
            }
            break;
            
        case IDC_REMOVE:
            {
                int iPos = (int)SendDlgItemMessage(hDlg, IDC_CURRENT, LB_GETCURSEL, 0, 0);
                
                LBMoveButton(lpad, IDC_CURRENT, iPos, IDC_BUTTONLIST, 0, 0);
                break;
            }
            
        case IDC_MOVEUP:
        case IDC_MOVEDOWN:
            {
                int iPosSrc, iPosDst;
                
                iPosSrc = (int)SendDlgItemMessage(hDlg, IDC_CURRENT, LB_GETCURSEL, 0, 0L);
                if (wParam == IDC_MOVEUP)
                    iPosDst = GetNearestInsert(lpad->ptb, iPosSrc - 1, 0, GNI_LOW);
                else
                    iPosDst = GetNearestInsert(lpad->ptb, iPosSrc + 2, lpad->ptb->iNumButtons, GNI_HIGH);
                
                LBMoveButton(lpad, IDC_CURRENT, iPosSrc, IDC_CURRENT,iPosDst,0);
                break;
            }
            
        case IDC_RESET:
            {
                // ptb will change across call below
                HWND hwndT = lpad->ptb->ci.hwnd;
                BOOL fClose = FALSE;
                NMTBCUSTOMIZEDLG nm;
                nm.hDlg = hDlg;
                if (CCSendNotify(&lpad->ptb->ci, TBN_RESET, &nm.hdr) == TBNRF_ENDCUSTOMIZE)
                    fClose = TRUE;
                
                // ptb probably changed across above call
                lpad->ptb = FixPTB(hwndT);
            
                /* Reset the dialog, but exit if something goes wrong. */
                lpad->iPos = 0;
                if (!fClose && InitAdjustDlg(hDlg, lpad))
                    break;
            }
            
            /* We have to fall through because we won't know where to insert
             * buttons after resetting.
             */
        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
            
        default:
            return(FALSE);
        }
        break;
        
    default:
        if (uMsg == uDragListMsg)
            return HandleDragMsg(lpad, hDlg, wParam, (LPDRAGLISTINFO)lParam);
        
        return(FALSE);
    }
    
    return(TRUE);
}

// BUGBUG: this should support saving to an IStream

/* This saves the state of the toolbar.  Spaces are saved as -1 (-2 if hidden)
 * and other buttons are just saved as the command ID.  When restoring, all
 * ID's are filled in, and the app is queried for all buttons so that the
 * bitmap and state information may be filled in.  Button ID's that are not
 * returned from the app are removed.
 */

BOOL SaveRestoreFromReg(PTBSTATE ptb, BOOL bWrite, HKEY hkr, LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    BOOL bRet = FALSE;
    TCHAR szDesc[128];
    
    if (bWrite)
    {
        UINT uSize = ptb->iNumButtons * sizeof(DWORD);
        NMTBSAVE nmtbs;
        BOOL fAlloced = FALSE;
        nmtbs.pData = NULL;
        nmtbs.cbData = uSize;
        nmtbs.pCurrent = NULL;
        nmtbs.iItem = -1; // signal pre saving
        nmtbs.cButtons = ptb->iNumButtons;
        CCSendNotify(&ptb->ci, TBN_SAVE, &nmtbs.hdr);
        if (!nmtbs.pData) {
            nmtbs.pData = (DWORD *)LocalAlloc(LPTR, nmtbs.cbData);
            fAlloced = TRUE;
        }

        // BUGBUG -- Somebody could've changed ptb->iNumButtons
        // during the CCSendNotify

        if (!nmtbs.pCurrent)
            nmtbs.pCurrent = nmtbs.pData;
        
        if (nmtbs.pData)
        {
            HKEY hkeySave;
            if (RegCreateKey(hkr, pszSubKey, &hkeySave) == ERROR_SUCCESS)
            {
                int i;
                for (i = 0; i < ptb->iNumButtons; i++)
                {
                    if (ptb->Buttons[i].idCommand)
                        *nmtbs.pCurrent = ptb->Buttons[i].idCommand;
                    else
                    {
                        // If the separator has an ID, then it is an "owner" item.
                        if (ptb->Buttons[i].fsState & TBSTATE_HIDDEN)
                            *nmtbs.pCurrent = (DWORD)-2;   // hidden
                        else
                            *nmtbs.pCurrent = (DWORD)-1;   // normal seperator
                    }
                    nmtbs.pCurrent++;
                    nmtbs.iItem = i;
                    TBOutputStruct(ptb, &ptb->Buttons[i], &nmtbs.tbButton);
                    CCSendNotify(&ptb->ci, TBN_SAVE, &nmtbs.hdr);
                }
                if (RegSetValueEx(hkeySave, (LPTSTR)pszValueName, 0, REG_BINARY, (LPVOID)nmtbs.pData, nmtbs.cbData) == ERROR_SUCCESS)
                    bRet = TRUE;
                RegCloseKey(hkeySave);
            }
            
            if (fAlloced)
                LocalFree((HLOCAL)nmtbs.pData);
        }
    }
    else
    {
        HKEY hkey;
        
        if (RegOpenKey(hkr, pszSubKey, &hkey) == ERROR_SUCCESS)
        {
            DWORD cbSize = 0;
            
            if ((RegQueryValueEx(hkey, (LPTSTR)pszValueName, 0, NULL, NULL, &cbSize) == ERROR_SUCCESS) &&
                (cbSize > sizeof(DWORD)))
            {
                UINT uSize = (UINT)cbSize;
                DWORD *pData = (DWORD *)LocalAlloc(LPTR, uSize);
                if (pData)
                {
                    DWORD dwType;
                    DWORD cbSize = (DWORD)uSize;
                    
                    if ((RegQueryValueEx(hkey, (LPTSTR)pszValueName, 0, &dwType, (LPVOID)pData, &cbSize) == ERROR_SUCCESS) &&
                        (dwType == REG_BINARY) &&
                        (cbSize == (DWORD)uSize))
                    {
                        int iButtonIndex;

                        NMTBRESTORE nmtbs;
                        BOOL fAlloced = FALSE;
                        nmtbs.pData = pData;
                        nmtbs.pCurrent = pData;
                        nmtbs.iItem = -1; // signal pre saving
                        nmtbs.cButtons = (int)uSize / SIZEOF(DWORD);
                        nmtbs.cbBytesPerRecord = SIZEOF(DWORD);
                        nmtbs.cbData = uSize;
                        // since we don't know the cButtons if they've added on extra data to pData,
                        // we'll use whatever they fill for cButtons
                        if (!CCSendNotify(&ptb->ci, TBN_RESTORE, &nmtbs.hdr)) {

                            //
                            // Before reloading the buttons, delete the tooltips
                            // of the previous buttons (if they exist).
                            //
                            if (ptb && ptb->hwndToolTips) {
                                TOOLINFO ti;

                                ti.cbSize = sizeof(ti);
                                ti.hwnd = ptb->ci.hwnd;

                                for (iButtonIndex = 0;
                                     iButtonIndex < ptb->iNumButtons; iButtonIndex++) {

                                    if (!(ptb->Buttons[iButtonIndex].fsStyle & TBSTYLE_SEP)) {
                                        ti.uId = ptb->Buttons[iButtonIndex].idCommand;
                                        SendMessage(ptb->hwndToolTips, TTM_DELTOOL,
                                            0, (LPARAM)(LPTOOLINFO)&ti);
                                    }
                                }
                            }

                            // BUGBUG -- can ptb be NULL here? - raymondc
                            // BUGBUG -- what if pCaptureButton != NULL?

                            // grow (or maybe shrink) pbt to hold new buttons
                            if (TBReallocButtons(ptb, nmtbs.cButtons))
                            {
                                int i;
                                if (ptb->iNumButtons < nmtbs.cButtons)
                                    ZeroMemory(&ptb->Buttons[ptb->iNumButtons], (nmtbs.cButtons - ptb->iNumButtons) * sizeof(TBBUTTON));
                                ptb->iNumButtons = nmtbs.cButtons;

                                for (i = 0; i < ptb->iNumButtons; i++)
                                {
                                    nmtbs.iItem = i;

                                    if ((long)*nmtbs.pCurrent < 0)
                                    {
                                        ptb->Buttons[i].fsStyle = TBSTYLE_SEP;
                                        ptb->Buttons[i].DUMMYUNION_MEMBER(iBitmap) = g_dxButtonSep;
                                        ptb->Buttons[i].idCommand = 0;
                                        if (*nmtbs.pCurrent == (DWORD)-1)
                                            ptb->Buttons[i].fsState = 0;
                                        else
                                        {
                                            ASSERT(*nmtbs.pCurrent == (DWORD)-2);
                                            ptb->Buttons[i].fsState = TBSTATE_HIDDEN;
                                        }
                                    }
                                    else
                                    {
                                        ptb->Buttons[i].fsStyle = 0;
                                        ptb->Buttons[i].idCommand = *nmtbs.pCurrent;
                                        ptb->Buttons[i].DUMMYUNION_MEMBER(iBitmap) = -1;
                                    }
                                    
                                    nmtbs.pCurrent++;
                                    
                                    TBOutputStruct(ptb, &ptb->Buttons[i], &nmtbs.tbButton);
                                    CCSendNotify(&ptb->ci, TBN_RESTORE, &nmtbs.hdr);
                                    ASSERT(nmtbs.tbButton.iString == -1 || !HIWORD(nmtbs.tbButton.iString));
                                    // we don't thunk.  only allow string index in string pool here
                                    if (HIWORD(nmtbs.tbButton.iString))
                                        nmtbs.tbButton.iString = 0;
                                    TBInputStruct(ptb, &ptb->Buttons[i], &nmtbs.tbButton);
                                }

                                // Now query for all buttons, and fill in the rest of the info

                                // For backward compatibility, ignore return value of TBN_BEGINADJUST
                                // if client is older than version 5 (NT5 #185499).
                                if (!SendCmdNotify(ptb, TBN_BEGINADJUST) || (ptb->ci.iVersion < 5)) {
                                    for (i = 0; ; i++)
                                    {
                                        TBBUTTONDATA tbAdjust;

                                        tbAdjust.idCommand = 0;

                                        if (!GetAdjustInfo(ptb, i, &tbAdjust, szDesc, ARRAYSIZE(szDesc)))
                                            break;

                                        if (!(tbAdjust.fsStyle & TBSTYLE_SEP) || tbAdjust.idCommand)
                                        {
                                            int iPos = PositionFromID(ptb, tbAdjust.idCommand);
                                            if (iPos >= 0) {
                                                ptb->Buttons[iPos] = tbAdjust;

                                            }
                                        }

                                    }
                                    SendCmdNotify(ptb, TBN_ENDADJUST);
                                }

                                // cleanup all the buttons that were not recognized
                                // do this backwards to minimize data movement (and nmtbs.cButtons changes)
                                for (i = ptb->iNumButtons - 1; i >= 0; i--)
                                {
                                    // DeleteButton does no realloc, so ptb will not move
                                    if (ptb->Buttons[i].DUMMYUNION_MEMBER(iBitmap) < 0)
                                        DeleteButton(ptb, (UINT)i);
                                    else {
                                        // the rest, add to tooltips 
                                        if(ptb->hwndToolTips &&
                                          (!(ptb->Buttons[i].fsStyle & TBSTYLE_SEP || !ptb->Buttons[i].idCommand))) {
                                            TOOLINFO ti;
                                            // don't bother setting the rect because we'll do it below
                                            // in TBInvalidateItemRects;
                                            ti.cbSize = sizeof(ti);
                                            ti.uFlags = 0;
                                            ti.hwnd = ptb->ci.hwnd;
                                            ti.uId = ptb->Buttons[i].idCommand;
                                            ti.lpszText = LPSTR_TEXTCALLBACK;

                                            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
                                        }
                                    }

                                }
                                bRet = (ptb->iNumButtons != 0); // success

                                // bugbug: break autosize to a function and call it
                                SendMessage(ptb->ci.hwnd, TB_AUTOSIZE, 0, 0);
                                InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
                                TBInvalidateItemRects(ptb);
                            }
                        }
                    }
                    LocalFree((HLOCAL)pData);
                }
            }
            RegCloseKey(hkey);
        }
    }
    
    return bRet;
}

#ifndef WIN32

#ifndef WINNT
#pragma data_seg(DATASEG_READONLY)
#endif
#ifdef WINNT
const TCHAR c_szToolbarStates[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ToolbarState");
#else
const TCHAR c_szToolbarStates[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ToolbarState");
#endif
#ifndef WINNT
#pragma data_seg()
#endif
BOOL SaveRestore(PTBSTATE ptb, BOOL bWrite, LPTSTR *lpNames)
{
    // note, we ignore lpNames[1] (the ini file name)
    // ... we hope we don't get conflicts with lpNames[0] entires
    
    return SaveRestoreFromReg(ptb, bWrite, HKEY_CURRENT_USER, c_szToolbarStates, lpNames[0]);
}

#endif


void CustomizeTB(PTBSTATE ptb, int iPos)
{
    ADJUSTDLGDATA ad;
    HWND hwndT = ptb->ci.hwnd;  // ptb will change across call below
    HRSRC hrsrc;
    LANGID wLang;
    LPVOID pTemplate;

    if (ptb->hdlgCust)      // We are already customizing this toolbar
        return;
    
    ad.ptb = ptb;
    ad.iPos = iPos;
    
    // REVIEW: really should be per thread data, but not likely to cause a problem
    
    // see note in WM_MEASUREITEM code
    g_dyButtonHack = (ptb->ci.style & TBSTYLE_FLAT) ? ptb->iDyBitmap : ptb->iButHeight;
    
    SendCmdNotify(ptb, TBN_BEGINADJUST);

    //
    //  Do locale-specific futzing.
    //
    wLang = LANGIDFROMLCID(CCGetProperThreadLocale(NULL));
    hrsrc = FindResourceExRetry(HINST_THISDLL, RT_DIALOG, MAKEINTRESOURCE(ADJUSTDLG), wLang);
    if (hrsrc &&
        (pTemplate = (LPVOID)LoadResource(HINST_THISDLL, hrsrc)))
    {
        DialogBoxIndirectParam(HINST_THISDLL, pTemplate,
                   ptb->ci.hwndParent, AdjustDlgProc, (LPARAM)(LPADJUSTDLGDATA)&ad);
    }

    // ptb probably changed across above call
    ptb = (PTBSTATE)GetWindowInt(hwndT, 0);
    ptb->hdlgCust = NULL;
    
    SendCmdNotify(ptb, TBN_ENDADJUST);
}

