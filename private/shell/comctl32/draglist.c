#include "ctlspriv.h"

#define DF_ACTUALLYDRAG	0x0001
#define DF_DEFERRED	0x0002

#define INITLINESPERSECOND	6
#define VERTCHANGENUMLINES	25

#define TIMERID		238
#define TIMERLEN	50

#define DX_INSERT	16
#define DY_INSERT	16


typedef struct {
    HWND hwndDrag;
    UINT uFlags;
} DRAGPROP, *PDRAGPROP;

UINT uDragListMsg = 0;
#ifndef WINNT
#pragma data_seg(DATASEG_READONLY)
#endif
const TCHAR szDragListMsgString[] = DRAGLISTMSGSTRING;
#ifndef WINNT
#pragma data_seg()
#endif

BOOL NEAR PASCAL PtInLBItem(HWND hLB, int nItem, POINT pt, int xInflate, int yInflate)
{
  RECT rc;

  if (nItem < 0)
      nItem = (int)SendMessage(hLB, LB_GETCURSEL, 0, 0L);

  if (SendMessage(hLB, LB_GETITEMRECT, nItem, (LPARAM)(LPRECT)&rc) == LB_ERR)
      return(FALSE);

  InflateRect(&rc, xInflate, yInflate);

  return(PtInRect(&rc, pt));
}


/*
 * DragListSubclassProc
 * --------------------
 *
 * Window procedure for subclassed list boxes
 */
LRESULT CALLBACK DragListSubclassProc(HWND hLB, UINT uMsg, WPARAM wParam,
      LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
  PDRAGPROP pDragProp;
  DRAGLISTINFO sNotify;
  BOOL bDragging;
  POINT pt;

  pDragProp = (PDRAGPROP)dwRefData;
  bDragging = pDragProp->hwndDrag == hLB;

  switch (uMsg)
    {
      case WM_NCDESTROY:
        if (bDragging)
            SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L);	/* cancel drag */

        RemoveWindowSubclass(hLB, DragListSubclassProc, 0);

        if (pDragProp)
            LocalFree((HLOCAL)pDragProp);
        break;

      case WM_LBUTTONDOWN:
        {
          int nItem;

          if (bDragging)				/* nested button-down */
              SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L);	/* cancel drag */

          SetFocus(hLB);

          pt.x = GET_X_LPARAM(lParam);
          pt.y = GET_Y_LPARAM(lParam);
          ClientToScreen(hLB, &pt);
          nItem = LBItemFromPt(hLB, pt, FALSE);

          if (nItem >= 0)
            {
              SendMessage(hLB, LB_SETCURSEL, nItem, 0L);
              if (GetWindowLong(hLB, GWL_STYLE) & LBS_NOTIFY)
                  SendMessage(GetParent(hLB), WM_COMMAND,
                              GET_WM_COMMAND_MPS(GetDlgCtrlID(hLB), hLB, LBN_SELCHANGE));
              sNotify.uNotification = DL_BEGINDRAG;
              goto QueryParent;
            }
          else
              goto FakeDrag;
        }

      case WM_TIMER:
        if (wParam != TIMERID)
            break;
        lParam = GetMessagePosClient(hLB, &pt);


        // fall through
      case WM_MOUSEMOVE:
	if (bDragging)
	  {
	    HWND hwndParent;
	    LRESULT lResult;

	    /* We may be just simulating a drag, but not actually doing
	     * anything.
	     */
	    if (!(pDragProp->uFlags&DF_ACTUALLYDRAG))
		return(0L);

	    /* We don't want to do any dragging until the user has dragged
	     * outside of the current selection.
	     */
	    if (pDragProp->uFlags & DF_DEFERRED)
	      {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
		if (PtInLBItem(hLB, -1, pt, 0, 4))
		    return 0;
		pDragProp->uFlags &= ~DF_DEFERRED;
	      }

	    sNotify.uNotification = DL_DRAGGING;

QueryParent:
	    hwndParent = GetParent(hLB);
	    sNotify.hWnd = hLB;

            sNotify.ptCursor.x = GET_X_LPARAM(lParam);
            sNotify.ptCursor.y = GET_Y_LPARAM(lParam);
	    ClientToScreen(hLB, &sNotify.ptCursor);

            lResult = SendMessage(hwndParent, uDragListMsg, GetDlgCtrlID(hLB),
		  (LPARAM)(LPDRAGLISTINFO)&sNotify);

	    if (uMsg == WM_LBUTTONDOWN)
	      {
		/* Some things may not be draggable
		 */
		if (lResult)
		  {
		    SetTimer(hLB, TIMERID, TIMERLEN, NULL);
		    pDragProp->uFlags = DF_DEFERRED | DF_ACTUALLYDRAG;
		  }
		else
		  {
FakeDrag:
		    pDragProp->uFlags = 0;
		  }

		/* Set capture and change mouse cursor
		 */
		pDragProp->hwndDrag = hLB;

		SetCapture(hLB);
	      }
	    else
	      {
		switch (lResult)
		  {
		    case DL_STOPCURSOR:
                      SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_NO)));
		      break;

		    case DL_COPYCURSOR:
                      SetCursor(LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_COPY)));
		      break;

		    case DL_MOVECURSOR:
                      SetCursor(LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_MOVE)));
		      break;

		    default:
		      break;
		  }
	      }

	    /* Don't call the def proc, since it may try to change the
	     * selection or set timers or things like that.
	     */
	    return(0L);
	  }
	break;

      case  WM_RBUTTONDOWN:
      case  WM_LBUTTONUP:
	/* if we are capturing mouse - release it and check for acceptable place
	 * where mouse is now to decide drop or not
	 */
	if (bDragging)
	  {
	    HWND hwndParent;

	    pDragProp->hwndDrag = NULL;
	    KillTimer(hLB, TIMERID);
	    ReleaseCapture();
	    SetCursor(LoadCursor(NULL, IDC_ARROW));

	    hwndParent = GetParent(hLB);

	    sNotify.uNotification = uMsg==WM_LBUTTONUP ? DL_DROPPED : DL_CANCELDRAG;
	    sNotify.hWnd = hLB;
            sNotify.ptCursor.x = GET_X_LPARAM(lParam);
            sNotify.ptCursor.y = GET_Y_LPARAM(lParam);
	    ClientToScreen(hLB, &sNotify.ptCursor);

	    SendMessage(hwndParent, uDragListMsg, GetDlgCtrlID(hLB),
		  (LPARAM)(LPDRAGLISTINFO)&sNotify);

	    /* We need to make sure to return 0 in case this is from a
	     * keyboard message.
	     */
	    return(0L);
	  }
	break;

      case WM_GETDLGCODE:
	if (bDragging)
          {
            return (DefSubclassProc(hLB, uMsg, wParam, lParam) |
                DLGC_WANTMESSAGE);
          }
	break;

      case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
          {
            SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L);
          }
        // fall through
      case WM_CHAR:
      case WM_KEYUP:
	/* We don't want the listbox processing this if we are dragging.
	 */
	if (bDragging)
	    return(0L);
	break;

      default:
	break;
    }

  return(DefSubclassProc(hLB, uMsg, wParam, lParam));
}


BOOL WINAPI MakeDragList(HWND hLB)
{
  PDRAGPROP pDragProp;

  if (!uDragListMsg)
      uDragListMsg = RegisterWindowMessage(szDragListMsgString);

  /* Check that we have not already subclassed this window.
   */
  if (GetWindowSubclass(hLB, DragListSubclassProc, 0, NULL))
      return(TRUE);

  pDragProp = (PDRAGPROP)LocalAlloc(LPTR, sizeof(DRAGPROP));
  if (!pDragProp)
      return(FALSE);

  if (!SetWindowSubclass(hLB, DragListSubclassProc, 0, (DWORD_PTR)pDragProp))
  {
      LocalFree((HLOCAL)pDragProp);
      return(FALSE);
  }

  return(TRUE);
}


int WINAPI LBItemFromPt(HWND hLB, POINT pt, BOOL bAutoScroll)
{
  static LONG dwLastScroll = 0;

  RECT rc;
  DWORD dwNow;
  int nItem;
  WORD wScrollDelay, wActualDelay;

  ScreenToClient(hLB, &pt);
  GetClientRect(hLB, &rc);

  nItem = (int)SendMessage(hLB, LB_GETTOPINDEX, 0, 0L);

  /* Is the point in the LB client area?
   */
  if (PtInRect(&rc, pt))
    {
      /* Check each visible item in turn.
       */
      for ( ; ; ++nItem)
	{
	  if (SendMessage(hLB, LB_GETITEMRECT, nItem, (LPARAM)(LPRECT)&rc)
		== LB_ERR)
	      break;

	  if (PtInRect(&rc, pt))
	      return(nItem);
	}
    }
  else
    {
      /* If we want autoscroll and the point is directly above or below the
       * LB, determine the direction and if it is time to scroll yet.
       */
      if (bAutoScroll && (UINT)pt.x<(UINT)rc.right)
	{
	  if (pt.y <= 0)
	    {
	      --nItem;
	    }
	  else
	    {
	      ++nItem;
	      pt.y = rc.bottom - pt.y;
	    }
	  wScrollDelay = (WORD)(1000 /
		(INITLINESPERSECOND - pt.y/VERTCHANGENUMLINES));

	  dwNow = GetTickCount();
	  wActualDelay = (WORD)(dwNow - dwLastScroll);

	  if (wActualDelay > wScrollDelay)
	    {
	      /* This will the actual number of scrolls per second to be
	       * much closer to the required number.
	       */
	      if (wActualDelay > wScrollDelay * 2)
		  dwLastScroll = dwNow;
	      else
		  dwLastScroll += wScrollDelay;

	      SendMessage(hLB, LB_SETTOPINDEX, nItem, 0L);
	    }
	}
    }

  return(-1);
}


void WINAPI DrawInsert(HWND hwndParent, HWND hLB, int nItem)
{
  static POINT ptLastInsert;
  static int nLastInsert = -1;

  RECT rc;

  /* Erase the old mark if necessary
   */
  if (nLastInsert>=0 && nItem!=nLastInsert)
    {
      rc.left = ptLastInsert.x;
      rc.top = ptLastInsert.y;
      rc.right = rc.left + DX_INSERT;
      rc.bottom = rc.top + DY_INSERT;

      /* Need to update immediately in case the insert rects overlap.
       */
      InvalidateRect(hwndParent, &rc, TRUE);
      UpdateWindow(hwndParent);

      nLastInsert = -1;
    }

  /* Draw a new mark if necessary
   */
  if (nItem!=nLastInsert && nItem>=0)
    {
      HICON hInsert = NULL;

      if (!hInsert)
	  hInsert = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_INSERT));

      if (hInsert)
	{
	  HDC hDC;

	  GetWindowRect(hLB, &rc);
	  ScreenToClient(hLB, (LPPOINT)&rc);
	  ptLastInsert.x = rc.left - DX_INSERT;

	  SendMessage(hLB, LB_GETITEMRECT, nItem, (LPARAM)(LPRECT)&rc);
	  ptLastInsert.y = rc.top - DY_INSERT/2;

	  nLastInsert = nItem;

	  ClientToScreen(hLB, &ptLastInsert);
	  ScreenToClient(hwndParent, &ptLastInsert);

	  hDC = GetDC(hwndParent);
	  DrawIcon(hDC, ptLastInsert.x, ptLastInsert.y, hInsert);
	  ReleaseDC(hwndParent, hDC);
	}
    }
}
