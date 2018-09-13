#include "ctlspriv.h"
#ifdef IEWIN31_25
#include "toolbar2.h"
#else
#include "toolbar.h"
#endif
#include "..\inc\help.h" // Help IDs

#ifdef WIN32
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    (void)SendMessage((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))
#else
    // dont cast result to void since we depend on this hack to get a handle back
    // from some WM_COMMAND messages
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    SendMessage((hwnd), WM_COMMAND, (WPARAM)(int)(id), MAKELPARAM((UINT)(hwndCtl), (codeNotify)))
#endif

void FAR PASCAL FlushButtonCache(PTBSTATE ptb);

#define SPACESTRLEN	20

#define FLAG_NODEL	0x8000
#define FLAG_HIDDEN	0x4000
#define FLAG_SEP	0x2000
#define FLAG_ALLFLAGS	(FLAG_NODEL|FLAG_HIDDEN|FLAG_SEP)

typedef struct {		/* instance data for toolbar edit dialog */
    HWND hDlg;			/* dialog hwnd */
    PTBSTATE ptb;		// current toolbar state
    int iPos;			/* position to insert into */
} ADJUSTDLGDATA, FAR *LPADJUSTDLGDATA;


int g_dyButtonHack = 0;		// to pass on before WM_INITDIALOG


int NEAR PASCAL GetPrevButton(PTBSTATE ptb, int iPos)
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

BOOL NEAR PASCAL GetAdjustInfo(PTBSTATE ptb, int iItem, LPTBBUTTON ptbButton, LPSTR lpString, int cbString)
{
#ifdef WIN32
    TBNOTIFY tbn;

    tbn.pszText = lpString;
    tbn.cchText = cbString;
    tbn.iItem = iItem;

    if (lpString)
	*lpString = 0;

    if ((BOOL)SendNotify(ptb->hwndCommand, ptb->hwnd, TBN_GETBUTTONINFO, &tbn.hdr))
    {
	*ptbButton = tbn.tbButton;
	return TRUE;
    }
    return FALSE;

#else
    LPADJUSTINFO lpai;
    // for compatibility with old users of we do this bogus stuff with WM_COMMAND
    HGLOBAL hInfo = (HANDLE)(int)SEND_WM_COMMAND(ptb->hwndCommand, GetWindowID(ptb->hwnd), iItem, TBN_ADJUSTINFO);
    if (!hInfo)
	return FALSE;

    // param validation...
    lpai = (LPADJUSTINFO)GlobalLock(hInfo);
    if (lpai)
    {
        TBInputStruct(ptb, ptbButton, &lpai->tbButton);
        if (lpString)
            lstrcpyn(lpString, (LPSTR)lpai + ptb->uStructSize, cbString);

	GlobalUnlock(hInfo);
	return TRUE;
    }

    return FALSE;
#endif
}

BOOL FAR PASCAL SendItemNotify(PTBSTATE ptb, int iItem, int code)
{
#ifdef WIN32
    TBNOTIFY tbn;
    tbn.iItem = iItem;
    // default return from SendNotify is false
    return SendNotify(ptb->hwndCommand, ptb->hwnd, code, &tbn.hdr);
#else
    // for compatibility with old users of we do this bogus stuff with WM_COMMAND
    // stuffing in an item index in a field that should be an hwnd
    return SEND_WM_COMMAND(ptb->hwndCommand, GetWindowID(ptb->hwnd), (HWND)iItem, code);
#endif
}

void NEAR PASCAL SendCmdNotify(PTBSTATE ptb, int code)
{
#ifdef WIN32
    NMHDR   hdr;
    SendNotify(ptb->hwndCommand, ptb->hwnd, code, &hdr);
#else
    SEND_WM_COMMAND(ptb->hwndCommand, GetWindowID(ptb->hwnd), ptb->hwnd, code);
#endif
}


// this is used to deal with the case where the ptb structure is re-alloced
// after a InsertButtons()

PTBSTATE NEAR PASCAL FixPTB(HWND hwnd)
{
    PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwnd, 0);

    if (ptb->hdlgCust)
    {
        LPADJUSTDLGDATA lpad = (LPADJUSTDLGDATA)GetWindowLong(ptb->hdlgCust, DWL_USER);
#ifdef DEBUG
        if (lpad->ptb != ptb)
	    DebugMsg(DM_TRACE, "Fixing busted ptb pointer");
#endif
	lpad->ptb = ptb;
    }
    return ptb;
}


void FAR PASCAL MoveButton(PTBSTATE ptb, int nSource)
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
  SetCapture(ptb->hwnd);

  // Get the dimension of the window.
  GetClientRect(ptb->hwnd, &rc);
  for ( ; ; )
    {
      while (!PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE))
	  ;

      if (GetCapture() != ptb->hwnd)
	  goto AbortMove;

      // See if the application wants to process the message...
      if (CallMsgFilter32(&msg32, MSGF_COMMCTRL_TOOLBARCUST, TRUE) != 0)
          continue;


      switch (msg32.message)
	{
	  case WM_KEYDOWN:
	  case WM_KEYUP:
	  case WM_CHAR:
	    break;

	  case WM_LBUTTONUP:
              RelayToToolTips(ptb->hwndToolTips, ptb->hwnd, msg32.message, msg32.wParam, msg32.lParam);
	    if (((short)HIWORD(msg32.lParam) > (short)(rc.bottom+ptb->iButWidth)) ||
		((short)LOWORD(msg32.lParam) > (rc.right+ptb->iButWidth)) ||
		((short)HIWORD(msg32.lParam) < -ptb->iButHeight/2) ||
		((short)LOWORD(msg32.lParam) < -ptb->iButWidth/2))
	      {
		/* If the button was dragged off the toolbar, delete it.
		 */
DeleteSrcButton:
		DeleteButton(ptb, nSource);
		SendCmdNotify(ptb, TBN_TOOLBARCHANGE);
                FlushToolTipsMgr(ptb);
	      }
	    else
	      {
	       TBBUTTON tbbAdd;

		/* Add half a button to X so that it looks like it is centered
		 * over the target button, iff we have a horizontal layout.
		 * Add half a button to Y otherwise.
		 */
		if (rc.right!=ptb->iButWidth)	
		    nDest = TBHitTest(ptb,
									 (short)LOWORD(msg32.lParam) + ptb->iButWidth / 2,
									 (short)HIWORD(msg32.lParam));
		else
		    nDest = TBHitTest(ptb,
									 (short)LOWORD(msg32.lParam),
									 (short)HIWORD(msg32.lParam) + ptb->iButHeight / 2);

		if (nDest < 0)
		    nDest = -1 - nDest;

		if (nDest>0 &&
			 (ptb->Buttons[nDest-1].fsState & TBSTATE_WRAP) &&
			 (short)LOWORD(msg32.lParam)>ptb->iButWidth &&
			 SendItemNotify(ptb, --nDest, TBN_QUERYINSERT))
		  {
		    tbbAdd = ptb->Buttons[nSource];
		    DeleteButton(ptb, nSource);
		    if (nDest>nSource)
			 {
				 --nDest;
			 }

			 /* Insert befor spaces, but after buttons. */
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
			tbbAdd.iBitmap = 0;
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

		    /* This is a normal move operation
		     */
		    tbbAdd = ptb->Buttons[nSource];

		    DeleteButton(ptb, nSource);
		    if (nDest > nSource)
			--nDest;
InsertSrcButton:
		    hwndT = ptb->hwnd;

		    InsertButtons(ptb, nDest, 1, &tbbAdd);

		    ptb = FixPTB(hwndT);

		    SendCmdNotify(ptb, TBN_TOOLBARCHANGE);
                    FlushToolTipsMgr(ptb);
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
  ReleaseCapture();
}


#define GNI_HIGH	0x0001
#define GNI_LOW		0x0002

int NEAR PASCAL GetNearestInsert(PTBSTATE ptb, int iPos, int iNumButtons, UINT uFlags)
{
  int i;
  BOOL bKeepTrying;

  /* Find the nearest index where we can actually insert items
   */
  for (i=iPos; ; ++i, --iPos)
    {
      bKeepTrying = FALSE;

      /* Notice we favor going high if both flags are set
       */
      if ((uFlags & GNI_HIGH) && i <= iNumButtons)
	{
	  bKeepTrying = TRUE;

	  if (SendItemNotify(ptb, i, TBN_QUERYINSERT))
	      return(i);
	}

      if ((uFlags & GNI_LOW) && iPos >= 0)
	{
	  bKeepTrying = TRUE;

	  if (SendItemNotify(ptb, i, TBN_QUERYINSERT))
	      return(iPos);
	}

      if (!bKeepTrying)
	  return(-1);	/* There was no place to add buttons. */
    }
}


BOOL NEAR PASCAL InitAdjustDlg(HWND hDlg, LPADJUSTDLGDATA lpad)
{
	HDC hDC;
	HFONT hFont;
	HWND hwndCurrent, hwndNew;
	PTBBUTTON ptbButton;
	int i, iPos, nItem, nWid, nMaxWid;
	TBBUTTON tbAdjust;
	char szDesc[128];

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

	for (i=0, ptbButton = lpad->ptb->Buttons; i < lpad->ptb->iNumButtons; ++i, ++ptbButton)
	{
		UINT uFlags;
		int iBitmap;

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
		}
		else
		{
			iBitmap = ptbButton->iBitmap;
		}

		/* Add the item and the data
		 * Note: A negative number in the LOWORD indicates no bitmap;
		 * otherwise it is the bitmap index.
		 */
		if ((int)SendMessage(hwndCurrent, LB_ADDSTRING, 0, (LPARAM)(LPSTR)c_szNULL) != i)
		{
			return(FALSE);
		}
		SendMessage(hwndCurrent, LB_SETITEMDATA, i, MAKELPARAM(iBitmap, uFlags));
	}

	/* Add a dummy "nodel" space at the end so things can be inserted at the end.
	 */
	if ((int)SendMessage(hwndCurrent, LB_ADDSTRING, 0,(LPARAM)(LPSTR)c_szNULL) == i)
	{
		SendMessage(hwndCurrent, LB_SETITEMDATA, i, MAKELPARAM(-1, FLAG_NODEL|FLAG_SEP));
	}

	/* Now add a space at the beginning of the "new" list.
	 */
	if (SendMessage(hwndNew, LB_ADDSTRING, 0, (LPARAM)(LPSTR)c_szNULL) == LB_ERR)
	{
		return(FALSE);
	}
	SendMessage(hwndNew, LB_SETITEMDATA, 0, MAKELPARAM(-1, FLAG_SEP));

	/* We need this to determine the widest (in pixels) item string.
	 */
	hDC = GetDC(hwndCurrent);
	hFont = (HFONT)(int)SendMessage(hwndCurrent, WM_GETFONT, 0, 0L);
	if (hFont)
	{
		hFont = SelectObject(hDC, hFont);
	}
	nMaxWid = 0;

	for (i=0; ; ++i)
	{
		// Get the info about the i'th item from the app.
		if (!GetAdjustInfo(lpad->ptb, i, &tbAdjust, szDesc, sizeof(szDesc)))
		    break;

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
				/* Don't show hidden buttons
				 */
				if (!(tbAdjust.fsState & TBSTATE_HIDDEN))
				{
					nItem = (int)SendMessage(hwndNew, LB_ADDSTRING, 0,
						(LPARAM)(LPSTR)szDesc);
					if (nItem != LB_ERR)
					{
						SendMessage(hwndNew, LB_SETITEMDATA, nItem,
							MAKELPARAM(tbAdjust.fsStyle & TBSTYLE_SEP
							? -1 : tbAdjust.iBitmap, i));
					}
				}
			}
			else
			/* The item is on the toolbar already */
			{
				/* Preserve the flags and bitmap.
				 */
				DWORD dwTemp = SendMessage(hwndCurrent, LB_GETITEMDATA, nItem, 0L);

				SendMessage(hwndCurrent, LB_DELETESTRING, nItem, 0L);

				if ((int)SendMessage(hwndCurrent, LB_INSERTSTRING, nItem,
					(LPARAM)(LPSTR)szDesc) != nItem)
				{
					ReleaseDC(hwndCurrent, hDC);
					return(FALSE);
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

void NEAR PASCAL PaintAdjustLine(PTBSTATE ptb, DRAWITEMSTRUCT FAR *lpdis)
{
  HDC hdc = lpdis->hDC;
  HWND hwndList = lpdis->hwndItem;
  PSTR pszText;
  RECT rc = lpdis->rcItem;
  int nBitmap, nLen, nItem = lpdis->itemID;
  COLORREF oldBkColor, oldTextColor;
  BOOL bSelected, bHasFocus;
  int wHeight;


  if (lpdis->CtlID != IDC_BUTTONLIST && lpdis->CtlID != IDC_CURRENT)
      return;

  nBitmap = LOWORD(lpdis->itemData);

  if (IsSeparator(lpdis->itemData))
      nLen = SPACESTRLEN;
  else
    {
      nLen = (int)SendMessage(hwndList, LB_GETTEXTLEN, nItem, 0L);
      if (nLen < 0)
	  return;
    }

  pszText = (PSTR)LocalAlloc(LPTR, nLen+1);
  if (!pszText)
      return;

  if (IsSeparator(lpdis->itemData))
      nLen = LoadString(HINST_THISDLL, IDS_SPACE, pszText, SPACESTRLEN);
  else
      SendMessage(hwndList, LB_GETTEXT, nItem, (LPARAM)(LPSTR)pszText);

  if (lpdis->itemAction != ODA_FOCUS)
    {
      COLORREF clr;
      char szSample[2];

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

      szSample[0] = 'W';

      MGetTextExtent(hdc, szSample, 1, NULL, &wHeight);

      ExtTextOut(hdc, rc.left + ptb->iButWidth + 2,
	    (rc.top + rc.bottom-wHeight) / 2,
	    ETO_CLIPPED | ETO_OPAQUE, &rc, pszText, nLen, NULL);

      /* We really care about the bitmap value here; this is not just an
       * indicator for the separator.
       */
      if (nBitmap >= 0)
	{
	  TBBUTTON tbbAdd;
	  HBITMAP hbmOldGlyphs, hbmOldMono;

	  tbbAdd.iBitmap = nBitmap;
	  tbbAdd.iString = -1;
	  tbbAdd.fsStyle = TBSTYLE_BUTTON;
	  tbbAdd.fsState = (BYTE)((HIWORD(lpdis->itemData) & FLAG_HIDDEN) ? 0 : TBSTATE_ENABLED);

	  // BUGBUG: we may need to enter a critical section around this (using globals)

	  // We need to kick-start the bitmap selection process.
	  ptb->nSelectedBM = -1;
	  hbmOldGlyphs = SelectBM(g_hdcGlyphs, ptb, 0);
	  if (!hbmOldGlyphs)
	      goto Error1;

	  hbmOldMono = SelectObject(g_hdcMono, g_hbmMono);

	  DrawButton(hdc, rc.left + 1, rc.top + 1,
		ptb->iButWidth, ptb->iButHeight, ptb, &tbbAdd, FALSE);

	  if (hbmOldMono)
	      SelectObject(g_hdcMono, hbmOldMono);
	  SelectObject(g_hdcGlyphs, hbmOldGlyphs);
Error1:
	  ;
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

  if (lpdis->itemAction == ODA_FOCUS || (lpdis->itemState & ODS_FOCUS))
      DrawFocusRect(hdc, &rc);

  LocalFree((HLOCAL)pszText);
}


void NEAR PASCAL LBMoveButton(LPADJUSTDLGDATA lpad, UINT wIDSrc, int iPosSrc,
      UINT wIDDst, int iPosDst, int iSelOffset)
{
  HWND hwndSrc, hwndDst;
  DWORD dwTemp;
  PSTR pStr;
  TBBUTTON tbAdjust;
  int iTopDst;

  hwndSrc = GetDlgItem(lpad->hDlg, wIDSrc);
  hwndDst = GetDlgItem(lpad->hDlg, wIDDst);

  /* Make sure we can delete the source and insert at the dest
   */
  dwTemp = SendMessage(hwndSrc, LB_GETITEMDATA, iPosSrc, 0L);
  if (iPosSrc < 0 || (HIWORD(dwTemp) & FLAG_NODEL))
      return;
  if (wIDDst == IDC_CURRENT &&
      !SendItemNotify(lpad->ptb, iPosDst, TBN_QUERYINSERT))
      return;

  /* Get the string for the source
   */
  pStr = (PSTR)LocalAlloc(LPTR,
	(int)(SendMessage(hwndSrc, LB_GETTEXTLEN, iPosSrc, 0L))+1);
  if (!pStr)
      return;
  SendMessage(hwndSrc, LB_GETTEXT, iPosSrc, (LPARAM)(LPSTR)pStr);

  SendMessage(hwndDst, WM_SETREDRAW, 0, 0L);
  iTopDst = (int)SendMessage(hwndDst, LB_GETTOPINDEX, 0, 0L);

  /* If we are inserting into the available button list, we need to determine
   * the insertion point
   */
  if (wIDDst == IDC_BUTTONLIST)
    {
      /* Insert this back in the available list if this is not a space or a
       * hidden button.
       */
      if (HIWORD(dwTemp)&(FLAG_SEP|FLAG_HIDDEN))
	{
	  iPosDst = 0;
	  goto DelTheSrc;
	}
      else
	{
	  UINT uTemp;

	  uTemp = HIWORD(dwTemp) & ~(FLAG_ALLFLAGS);

	  /* This just does a linear search for where to put the
	   * item.  Slow, but this only happens when the user clicks
	   * the "Remove" button.
	   */
	  for (iPosDst=1; ; ++iPosDst)
	    {
	      /* Notice that this will break out when iPosDst is
	       * past the number of items, since -1 will be returned
	       */
	      if ((UINT)HIWORD(SendMessage(hwndDst, LB_GETITEMDATA,
		    iPosDst, 0L)) >= uTemp)
		  break;
	    }
	}
    }
  else if (iPosDst < 0)
      goto CleanUp;

  /* Attempt to insert the new string
   */
  if ((int)SendMessage(hwndDst, LB_INSERTSTRING, iPosDst, (LPARAM)(LPSTR)pStr)
	== iPosDst)
    {
      /* Attempt to sync up the actual toolbar.
       */
      if (wIDDst == IDC_CURRENT)
	{
	  HWND hwndT;

	  if (IsSeparator(dwTemp))
	    {
	      // Make up a dummy lpInfo if this is a space
	      tbAdjust.iBitmap = 0;
	      tbAdjust.idCommand = 0;
	      tbAdjust.fsState = 0;
	      tbAdjust.fsStyle = TBSTYLE_SEP;
	    }
	  else
	    {
	      // callback to get info
	      if (!GetAdjustInfo(lpad->ptb, HIWORD(dwTemp) & ~(FLAG_ALLFLAGS), &tbAdjust, NULL, 0))
		  goto DelTheDst;
	    }

	  hwndT = lpad->ptb->hwnd;

	  if (!InsertButtons(lpad->ptb, iPosDst, 1, &tbAdjust))
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

      SendMessage(hwndDst, LB_SETITEMDATA, iPosDst, dwTemp);

DelTheSrc:
      /* Don't delete the "Separator" in the new list
       */
      if (wIDSrc!=IDC_BUTTONLIST || iPosSrc!=0)
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

      /* Delete the corresponding button
       */
      if (wIDSrc == IDC_CURRENT)
	  DeleteButton(lpad->ptb, iPosSrc);

      /* Only set the src index if the two windows are different
       */
      if (wIDSrc != wIDDst)
	{
	  if (SendMessage(hwndSrc, LB_SETCURSEL, iPosSrc, 0L) == LB_ERR)
	      SendMessage(hwndSrc, LB_SETCURSEL, iPosSrc-1, 0L);
	  SEND_WM_COMMAND(lpad->hDlg, wIDSrc, hwndSrc, LBN_SELCHANGE);
	}

      /* Send the final SELCHANGE message after everything else is done
       */
      SendMessage(hwndDst, LB_SETCURSEL, iPosDst+iSelOffset, 0L);
      SEND_WM_COMMAND(lpad->hDlg, wIDDst, hwndDst, LBN_SELCHANGE);

    }

CleanUp:

  LocalFree((HLOCAL)pStr);

  if (wIDSrc == wIDDst)
      SendMessage(hwndDst, LB_SETTOPINDEX, iTopDst, 0L);
  SendMessage(hwndDst, WM_SETREDRAW, 1, 0L);

  InvalidateRect(hwndDst, NULL, TRUE);

  SendCmdNotify(lpad->ptb, TBN_TOOLBARCHANGE);
}


void NEAR PASCAL SafeEnableWindow(HWND hDlg, UINT wID, HWND hwndDef, BOOL bEnable)
{
  HWND hwndEnable;

  hwndEnable = GetDlgItem(hDlg, wID);

  if (!bEnable && GetFocus()==hwndEnable)
      SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndDef, 1L);
  EnableWindow(hwndEnable, bEnable);
}

int NEAR PASCAL InsertIndex(LPADJUSTDLGDATA lpad, POINT pt, BOOL bDragging)
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


BOOL NEAR PASCAL IsInButtonList(HWND hDlg, POINT pt)
{
  ScreenToClient(hDlg, &pt);

  return(ChildWindowFromPoint(hDlg, pt) == GetDlgItem(hDlg, IDC_BUTTONLIST));
}


BOOL NEAR PASCAL HandleDragMsg(LPADJUSTDLGDATA lpad, HWND hDlg, WPARAM wID, LPDRAGLISTINFO lpns)
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


#pragma data_seg(DATASEG_READONLY)
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
#pragma data_seg()

BOOL CALLBACK AdjustDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LPADJUSTDLGDATA lpad = (LPADJUSTDLGDATA)GetWindowLong(hDlg, DWL_USER);
  switch (uMsg)
    {
      case WM_INITDIALOG:

	  SetWindowLong(hDlg, DWL_USER, lParam);  /* LPADJUSTDLGDATA pointer */
	  if (!InitAdjustDlg(hDlg, (LPADJUSTDLGDATA)lParam))
	      EndDialog(hDlg, FALSE);

	  ShowWindow(hDlg, SW_SHOW);
	  UpdateWindow(hDlg);
	  SetFocus(GetDlgItem(hDlg, IDC_CURRENT));

	  MakeDragList(GetDlgItem(hDlg, IDC_CURRENT));
	  MakeDragList(GetDlgItem(hDlg, IDC_BUTTONLIST));

	  return FALSE;

      case WM_MEASUREITEM:
#define lpmis ((MEASUREITEMSTRUCT FAR *)lParam)

	if (lpmis->CtlID == IDC_BUTTONLIST || lpmis->CtlID == IDC_CURRENT)
	{
	    int nHeight;
	    HWND hwndList = GetDlgItem(hDlg, lpmis->CtlID);
	    HDC hDC = GetDC(hwndList);
            char szSample[2];

	    szSample[0] = 'W';

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
	PaintAdjustLine(lpad->ptb, (DRAWITEMSTRUCT FAR *)lParam);
	break;

      /*case WM_HELP:
        SendCmdNotify(lpad->ptb, TBN_CUSTHELP);
        break;*/

      case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
          HELP_WM_HELP, (DWORD)(LPSTR) aAdjustHelpIDs);
        break;

      case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
          (DWORD)(LPVOID) aAdjustHelpIDs);
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
			    0, 0L), (LONG)(LPRECT)&rc) != LB_ERR)
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

		      SafeEnableWindow(hDlg, IDOK, hwndList, SendItemNotify(lpad->ptb, iPos, TBN_QUERYINSERT));

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
			    0, 0L), (LONG)(LPRECT)&rc) != LB_ERR)
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
		   HWND hwndT = lpad->ptb->hwnd;

		   SendCmdNotify(lpad->ptb, TBN_RESET);

		   // ptb probably changed across above call
		   lpad->ptb = FixPTB(hwndT);
		}

		/* Reset the dialog, but exit if something goes wrong. */
		lpad->iPos = 0;
		if (InitAdjustDlg(hDlg, lpad))
		    break;

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

BOOL FAR PASCAL SaveRestoreFromReg(PTBSTATE ptb, BOOL bWrite, HKEY hkr, LPCSTR pszSubKey, LPCSTR pszValueName)
{
    BOOL bRet = FALSE;

    if (bWrite)
    {
    	UINT uSize = ptb->iNumButtons * sizeof(DWORD);
        DWORD *pData = (DWORD *)LocalAlloc(LPTR, uSize);
    	if (pData)
	{
	    HKEY hkeySave;
    	    if (RegCreateKey(hkr, pszSubKey, &hkeySave) == ERROR_SUCCESS)
	    {
	        int i;
      	        for (i = 0; i < ptb->iNumButtons; i++)
	        {
	            if (ptb->Buttons[i].idCommand)
	                pData[i] = ptb->Buttons[i].idCommand;
	            else
	            {
	                // If the separator has an ID, then it is an "owner" item.
	                if (ptb->Buttons[i].fsState & TBSTATE_HIDDEN)
	       	            pData[i] = (DWORD)-2;	// hidden
	                else
	       	            pData[i] = (DWORD)-1;	// normal seperator
	            }
	        }
    	        if (RegSetValueEx(hkeySave, (LPSTR)pszValueName, 0, REG_BINARY, (LPVOID)pData, uSize) == ERROR_SUCCESS)
	   	    bRet = TRUE;
    	        RegCloseKey(hkeySave);
    	    }
            LocalFree((HLOCAL)pData);
	}
    }
    else
    {
	HKEY hkey;

        if (RegOpenKey(hkr, pszSubKey, &hkey) == ERROR_SUCCESS)
        {
	    DWORD cbSize = 0;

	    if ((RegQueryValueEx(hkey, (LPSTR)pszValueName, 0, NULL, NULL, &cbSize) == ERROR_SUCCESS) &&
		(cbSize > sizeof(DWORD)) &&
	        (cbSize < (512 * sizeof(DWORD)))) // sanity check...
	    {
		UINT uSize = (UINT)cbSize;
	    	DWORD *pData = (DWORD *)LocalAlloc(LPTR, uSize);
	        if (pData)
	        {
	            DWORD dwType;
	            DWORD cbSize = (DWORD)uSize;

	            if ((RegQueryValueEx(hkey, (LPSTR)pszValueName, 0, &dwType, (LPVOID)pData, &cbSize) == ERROR_SUCCESS) &&
		        (dwType == REG_BINARY) &&
	                (cbSize == (DWORD)uSize))
		    {
                        int iButtonIndex;
                        int iNumButtons = (int)uSize / sizeof(DWORD);
                        PTBSTATE pTemp;

                        //
                        // Before reloading the buttons, delete the tooltips
                        // of the previous buttons (if they exist).
                        //

                        if (ptb && ptb->hwndToolTips) {
                            TOOLINFO ti;

                            ti.cbSize = sizeof(ti);
                            ti.hwnd = ptb->hwnd;

                            for (iButtonIndex = 0;
                                 iButtonIndex < ptb->iNumButtons; iButtonIndex++) {

                                 if (!(ptb->Buttons[iButtonIndex].fsStyle & TBSTYLE_SEP)) {
                                     ti.uId = ptb->Buttons[iButtonIndex].idCommand;
                                     SendMessage(ptb->hwndToolTips, TTM_DELTOOL,
                                                 0, (LPARAM)(LPTOOLINFO)&ti);
                                 }
                             }
                        }

			// grow pbt to hold new buttons
		    	pTemp = (PTBSTATE)LocalReAlloc(ptb, sizeof(TBSTATE) + (iNumButtons - 1) * sizeof(TBBUTTON), LMEM_MOVEABLE);
	    	        if (pTemp)
			{
			    int i;
#ifdef DEBUG
			    if (ptb != pTemp)
				DebugMsg(DM_TRACE, "toolbar load, ptb changed on re-alloc");
#endif
	    	            ptb = pTemp;
	    	            SetWindowInt(ptb->hwnd, 0, (int)ptb);

	    	            ptb->iNumButtons = iNumButtons;

		      	    for (i = 0; i < ptb->iNumButtons; i++)
	        	    {
	    	      		if ((long)pData[i] < 0)
	    			{
	    		  	    ptb->Buttons[i].fsStyle = TBSTYLE_SEP;
	    		  	    ptb->Buttons[i].iBitmap = g_dxButtonSep;
	    		  	    ptb->Buttons[i].idCommand = 0;
	    		  	    if (pData[i] == (DWORD)-1)
	    		                ptb->Buttons[i].fsState = 0;
	    		  	    else
				    {
					Assert(pData[i] == (DWORD)-2);
	    		      		ptb->Buttons[i].fsState = TBSTATE_HIDDEN;
				    }
	    			}
	    	      		else
	    			{
	    		  	    ptb->Buttons[i].idCommand = pData[i];
	    		  	    ptb->Buttons[i].iBitmap = -1;
	    			}
	    	    	    }

	    	  	    // Now query for all buttons, and fill in the rest of the info
	    	  	    SendCmdNotify(ptb, TBN_BEGINADJUST);
	    	  	    for (i = 0; ; i++)
	    	    	    {
				TBBUTTON tbAdjust;

				tbAdjust.idCommand = 0;

	    	      		if (!GetAdjustInfo(ptb, i, &tbAdjust, NULL, 0))
	    	          	    break;

		    	        if (!(tbAdjust.fsStyle & TBSTYLE_SEP) || tbAdjust.idCommand)
	    			{
	    		  	    int iPos = PositionFromID(ptb, tbAdjust.idCommand);
	    		  	    if (iPos >= 0)
	    		      		ptb->Buttons[iPos] = tbAdjust;

                                    if(ptb->hwndToolTips) {
                                        TOOLINFO ti;
                                        // don't bother setting the rect because we'll do it below
                                        // in FlushToolTipsMgr;
                                        ti.cbSize = sizeof(ti);
                                        ti.uFlags = 0;
                                        ti.hwnd = ptb->hwnd;
                                        ti.uId =  ptb->Buttons[iPos].idCommand;
                                        ti.lpszText = LPSTR_TEXTCALLBACK;

                                        SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
                                    }

	    			}
	    	    	    }
	    	  	    SendCmdNotify(ptb, TBN_ENDADJUST);

			    // cleanup all the buttons that were not recognized
			    // do this backwards to minimize data movement (and iNumButtons changes)
	    	  	    for (i = ptb->iNumButtons - 1; i >= 0; i--)
	    	    	    {
	    	      		// DeleteButton does no realloc, so ptb will not move
	    	      		if (ptb->Buttons[i].iBitmap < 0)
	    		  	    DeleteButton(ptb, (UINT)i);
	    	    	    }
			    bRet = (ptb->iNumButtons != 0);	// success

                            FlushButtonCache(ptb);
                            // bugbug: break autosize to a function and call it
                            SendMessage(ptb->hwnd, TB_AUTOSIZE, 0,0);
                            InvalidateRect(ptb->hwnd, NULL, TRUE);
	                    FlushToolTipsMgr(ptb);
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

#pragma data_seg(DATASEG_READONLY)
const char c_szToolbarStates[] = "Software\\Microsoft\\Windows\\CurrentVersion\\ToolbarState";
#pragma data_seg()
BOOL FAR PASCAL SaveRestore(PTBSTATE ptb, BOOL bWrite, LPSTR FAR *lpNames)
{
    // note, we ignore lpNames[1] (the ini file name)
    // ... we hope we don't get conflicts with lpNames[0] entires

    return SaveRestoreFromReg(ptb, bWrite, HKEY_CURRENT_USER, c_szToolbarStates, lpNames[0]);
}

#endif


void FAR PASCAL CustomizeTB(PTBSTATE ptb, int iPos)
{
    ADJUSTDLGDATA ad;
    HWND hwndT = ptb->hwnd;	// ptb will change across call below

    if (ptb->hdlgCust)		// We are already customizing this toolbar
        return;

    ad.ptb = ptb;
    ad.iPos = iPos;

    // REVIEW: really should be per thread data, but not likely to cause a problem

    g_dyButtonHack = ptb->iButWidth;	// see note in WM_MEASUREITEM code

    SendCmdNotify(ptb, TBN_BEGINADJUST);

    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(ADJUSTDLG), ptb->hwndCommand,
    	AdjustDlgProc, (LPARAM)(LPADJUSTDLGDATA)&ad);

    // ptb probably changed across above call
    ptb = (PTBSTATE)GetWindowInt(hwndT, 0);
    ptb->hdlgCust = NULL;

    SendCmdNotify(ptb, TBN_ENDADJUST);
    // SendCmdNotify(ptb, TBN_TOOLBARCHANGE);
}
