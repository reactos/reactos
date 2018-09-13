/* File: D:\WACKER\tdll\termproc.c (Created: 06-Dec-1993)
 *
 * Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 * $Revision: 2 $
 * $Date: 2/05/99 3:21p $
 */

#include <windows.h>
#include <imm.h>
#pragma hdrstop
// #define  DEBUGSTR 1

#include "stdtyp.h"
#include "sf.h"
#include "sess_ids.h"
#include "assert.h"
#include "session.h"
#include "backscrl.h"
#include "timers.h"
#include "keyutil.h"
#include "chars.h"
#include "cloop.h"
#include "misc.h"
#include <emu\emu.h>
#include "term.h"
#include "term.hh"

// When parsing values for mouse moves, scrolls, etc. HIWORD and LOWORD
// clip the sign extension since these are 16 bit values.  These macros
// cast the result to a short so that sign extension works correctly

#define LOSHORT(x)   ((short)LOWORD(x))
#define HISHORT(x)   ((short)HIWORD(x))

// HORZPAGESIZE represents the size in columns of one page horizontally.
// We need this for two reasons.  1) We need a page size so we know how far
// to skip when user "pages" horizontally using the scrollbar. 2) So we
// have a size to set for the thumb size.  Ten is arbritary but seems
// about right. - mrw

#define HORZPAGESIZE 10

/* --- static function prototypes ---*/

static void TP_WM_CREATE(const HWND hwnd);

static void TP_WM_CHAR(const HWND hwnd, const UINT message,
                  const WPARAM wPar, const LPARAM lPar);

static void TP_WM_IME_CHAR(const HWND hwnd, const UINT message,
                     const WPARAM wPar, const LPARAM lPar);

static int kabExpandMacroKey( const HSESSION hSession, KEY_T aKey );
static void TP_WM_TERM_KEY(const HWND hwnd, KEY_T Key);
static void TP_WM_SETFOCUS(const HWND hwnd);
static void TP_WM_KILLFOCUS(const HWND hwnd);
static void TP_WM_VSCROLL(HWND hwnd, int nCode, int nPos, HWND hwndScrl);
static void TP_WM_HSCROLL(HWND hwnd, int nCode, int nPos, HWND hwndScrl);
static void TP_WM_TERM_TRACK(const HHTERM hhTerm, const int fForce);
static void TP_WM_EMU_SETTINGS(const HHTERM hhTerm);
static int TP_WM_TERM_LOAD_SETTINGS(const HHTERM hhTerm);
static int TP_WM_TERM_SAVE_SETTINGS(const HHTERM hhTerm);
static void TP_WM_TERM_FORCE_WMSIZE(const HHTERM hhTerm);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TermProc
 *
 * DESCRIPTION:
 * Terminal window
 *
 * ARGUMENTS:
 * Standard CALLBACK args
 *
 * RETURNS:
 * Standard CALLBACK codes
 *
 */
LRESULT CALLBACK TermProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
   {
   HHTERM hhTerm;

   switch (uMsg)
      {
      case WM_HELP:
         // We do not want start the help engine due to F1 when function
         // keys are being interpreted as terminal keys
         // (NOT Windows keys). JRJ 12/94
         //
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

         if(emuIsEmuKey(sessQueryEmuHdl(hhTerm->hSession),
               VIRTUAL_KEY | VK_F1))
            {
            return 0;
            }

         break;

      case WM_CREATE:
         TP_WM_CREATE(hwnd);
         return 0;

      case WM_SIZE:
         TP_WM_SIZE(hwnd, (unsigned)wPar, LOSHORT(lPar), HISHORT(lPar));
         return 0;

      case WM_IME_CHAR:
         TP_WM_IME_CHAR(hwnd, uMsg, wPar, lPar);
         return 0;

      case WM_KEYDOWN:
      case WM_CHAR:
         TP_WM_CHAR(hwnd, uMsg, wPar, lPar);
         return 0;

      case WM_PAINT:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termPaint(hhTerm, hwnd);
         return 0;

      case WM_SYSCOLORCHANGE:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termSysColorChng(hhTerm);
         return 0;

      case WM_SETFOCUS:
         TP_WM_SETFOCUS(hwnd);
         return 0;

      case WM_KILLFOCUS:
         TP_WM_KILLFOCUS(hwnd);
         return 0;

      case WM_VSCROLL:
         TP_WM_VSCROLL(hwnd, LOWORD(wPar), HISHORT(wPar), (HWND)lPar);
         return 0;

      case WM_HSCROLL:
         TP_WM_HSCROLL(hwnd, LOWORD(wPar), HISHORT(wPar), (HWND)lPar);
         return 0;

      case WM_LBUTTONDOWN:
         TP_WM_LBTNDN(hwnd, (unsigned)wPar, LOSHORT(lPar), HISHORT(lPar));
         return 0;

      case WM_MOUSEMOVE:
         TP_WM_MOUSEMOVE(hwnd, (unsigned)wPar, LOSHORT(lPar), HISHORT(lPar));
         return 0;

      case WM_LBUTTONUP:
         TP_WM_LBTNUP(hwnd, (unsigned)wPar, LOSHORT(lPar), HISHORT(lPar));
         return 0;

      case WM_LBUTTONDBLCLK:
         TP_WM_LBTNDBLCLK(hwnd, (unsigned)wPar, LOSHORT(lPar), HISHORT(lPar));
         return 0;

      case WM_DESTROY:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         DestroyTerminalHdl(hhTerm);
         hhTerm = NULL;
         return 0;

      /* --- Public terminal messages --- */

      case WM_TERM_GETUPDATE:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termGetUpdate(hhTerm, TRUE);
         return 0;

      case WM_TERM_BEZEL:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         hhTerm->xBezel = (hhTerm->xBezel) ? 0 : BEZEL_SIZE;

         // Need to recompute scrollbar min's and max's since the
         // presence/absence of the bezel affects these items.  This
         // is done in the WM_SIZE handler and can be called directly
         // with the current client size.

         TP_WM_SIZE(hwnd, SIZE_RESTORED, hhTerm->cx, hhTerm->cy);
         InvalidateRect(hwnd, 0, FALSE);
         return 0;

      case WM_TERM_Q_BEZEL:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         return hhTerm->xBezel;

      case WM_TERM_Q_SNAP:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termQuerySnapRect(hhTerm, (LPRECT)lPar);
         return 0;

      case WM_TERM_KEY:
         TP_WM_TERM_KEY(hwnd, (unsigned)wPar);
         return 0;

        case WM_TERM_CLRATTR:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termSetClrAttr(hhTerm);
         return 0;

      case WM_TERM_GETLOGFONT:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         *(PLOGFONT)lPar = hhTerm->lf;
         return 0;

      case WM_TERM_SETLOGFONT:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termSetFont(hhTerm, (PLOGFONT)lPar);
         return 0;

      case WM_TERM_Q_MARKED:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         return hhTerm->fMarkingLock;

      case WM_TERM_UNMARK:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         LinkCursors(hhTerm);
         return 0;

      case WM_TERM_MARK_ALL:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         MarkTextAll(hhTerm);
         return 0;

      case WM_TERM_TRACK:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         TP_WM_TERM_TRACK(hhTerm, (int)wPar);
         return 0;

      case WM_TERM_EMU_SETTINGS:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         TP_WM_EMU_SETTINGS(hhTerm);
         return 0;

      case WM_TERM_Q_MARKED_RANGE:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         *(PPOINT)wPar = hhTerm->ptBeg;
         *(PPOINT)lPar = hhTerm->ptEnd;
         return 0;
         
      case WM_TERM_LOAD_SETTINGS:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         TP_WM_TERM_LOAD_SETTINGS(hhTerm);
         return 0;
         
      case WM_TERM_SAVE_SETTINGS:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         TP_WM_TERM_SAVE_SETTINGS(hhTerm);
         return 0;

      case WM_TERM_FORCE_WMSIZE:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         TP_WM_TERM_FORCE_WMSIZE(hhTerm);
         return 0;
      
      /* --- Private terminal messages --- */

      case WM_TERM_SCRLMARK:
         MarkingTimerProc((void *)hwnd, 0);
         return 0;

      case WM_TERM_KLUDGE_FONT:
         hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
         termSetFont(hhTerm, &hhTerm->lfHold);
         RefreshTermWindow(hwnd);
         return 0;

      default:
         break;
      }

   return DefWindowProc(hwnd, uMsg, wPar, lPar);
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_CREATE
 *
 * DESCRIPTION:
 * Create message processor for terminal window.
 *
 * ARGUMENTS:
 * hwnd  - terminal window handle
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_CREATE(const HWND hwnd)
   {
   HHTERM hhTerm;
   SCROLLINFO scrinf;
   LONG_PTR ExStyle;

   ExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
   //
   // [mhamid] If it is mirrored then turn off mirroing.
   //
   if (ExStyle & WS_EX_LAYOUTRTL) {
       SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle & ~WS_EX_LAYOUTRTL);
   }
   // Create an internal handle for instance data storage.

   hhTerm = CreateTerminalHdl(hwnd);

   // Need to set this even if hTerm is 0 so WM_DESTROY works.

   SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hhTerm);

   if (hhTerm == 0)
      {
      assert(FALSE);
      ExitProcess(1);
      }

   scrinf.cbSize = sizeof(scrinf);
   scrinf.fMask = SIF_DISABLENOSCROLL;
   SetScrollInfo(hwnd, SB_VERT, &scrinf, 0);
   return;
   }

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_CMD
 *
 * DESCRIPTION:
 * WM_COMMAND processor for TermProc()
 *
 * ARGUMENTS:
 * hwnd     - terminal window handle
 * nId      - item, control, or accelerator identifier
 * nNotify  - notification code
 * hwndCtrl - handle of control
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_CMD(const HWND hwnd, const int nId, const int nNotify,
                 const HWND hwndCtrl)
   {
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   switch (nId)
      {
      case WM_USER:
         break;
      }

   return;
   }
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_SIZE
 *
 * DESCRIPTION:
 * WM_SIZE message processor for termproc.  If iWidth and iHite are zero,
 * then the window sizes itself to fit the session window accounting for
 * statusbars and toolbars.
 *
 * ARGUMENTS:
 * hwnd     - terminal window
 * fwSizeType  - from WM_SIZE
 * iWidth      - width of window
 * iHige    - hite of window
 *
 * RETURNS:
 * void
 *
 */
void TP_WM_SIZE(const HWND hwnd,
            const unsigned fwSizeType,
            const int iWidth,
            const int iHite)
   {
   RECT rc, rcTmp;
   int i, j, k;
   SCROLLINFO scrinf;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
   const HWND hwndToolbar = sessQueryHwndToolbar(hhTerm->hSession);
   const HWND hwndStatusbar = sessQueryHwndStatusbar(hhTerm->hSession);
   const HWND hwndSidebar = sessQuerySidebarHwnd(hhTerm->hSession);

   // If the window is hidden, we really don't need to do anything.

   if (!IsWindowVisible(hwnd))
      return;

   // If we get a message with 0, 0 for width, hite, make terminal window
   // fit the session window less the toolbar and statusbar if present.

   if (iWidth == 0 && iHite == 0)
      {
      GetClientRect(hhTerm->hwndSession, &rc);

      // Note: See if we can use sessQuery funcs for
      // this
      if (IsWindow(hwndToolbar) && IsWindowVisible(hwndToolbar))
         {
         GetWindowRect(hwndToolbar, &rcTmp);
         rc.top += (rcTmp.bottom - rcTmp.top);
         }

      if (IsWindow(hwndStatusbar) && IsWindowVisible(hwndStatusbar))
         {
         GetWindowRect(hwndStatusbar, &rcTmp);
         rc.bottom -= (rcTmp.bottom - rcTmp.top);
         rc.bottom += 2 * GetSystemMetrics(SM_CYBORDER);
         }

      if (IsWindow(hwndSidebar) && IsWindowVisible(hwndSidebar))
         {
         GetWindowRect(hwndSidebar, &rcTmp);
         rc.left += (rcTmp.right - rcTmp.left);
         }

      MoveWindow(hwnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
         TRUE);
      }

   // Compute size related items.

   else
      {
      hhTerm->cx = iWidth;
      hhTerm->cy = iHite;

      // Set Min's and Max's now.

      i = hhTerm->iTermHite;  // save old height for bottom line alignment
      hhTerm->iTermHite = hhTerm->cy / hhTerm->yChar;

      // If bezel is drawn, make sure we have room

      j = 0;

      if (hhTerm->xBezel)
         {
         if ((hhTerm->cy % hhTerm->yChar) < (hhTerm->xBezel + 1))
            j = 1;
         }

      hhTerm->iVScrlMin = min(-hhTerm->iBkLines,
         hhTerm->iRows - hhTerm->iTermHite + 1 + j);

      // Little hack here to make sure that if we were at the bottom
      // we show the bottom.

      k = (hhTerm->iVScrlMax == hhTerm->iVScrlPos);

      hhTerm->iVScrlMax = max(hhTerm->iVScrlMin, hhTerm->iRows + 1 + j
                        - hhTerm->iTermHite);

      // First time through set to bottom

      if (k)
         hhTerm->iVScrlPos = hhTerm->iVScrlMax;

      else
         hhTerm->iVScrlPos -= hhTerm->iTermHite - i;

      hhTerm->iVScrlPos = max(hhTerm->iVScrlPos, hhTerm->iVScrlMin);
      hhTerm->iVScrlPos = min(hhTerm->iVScrlPos, hhTerm->iVScrlMax);

      // hhTerm->iHScrlMax = #Cols - #Cols Showing
      hhTerm->iHScrlMax = max(0, hhTerm->iCols -
         ((hhTerm->cx - hhTerm->xIndent - (2 * hhTerm->xBezel))
            / hhTerm->xChar));

      hhTerm->iHScrlPos = min(hhTerm->iHScrlPos, hhTerm->iHScrlMax);

      // Check to see if we scrolled entirely into backscroll
      // region.  If so, set fBackscrlLock for this view.  And refill
      // its backscroll buffer (important).

      if ((hhTerm->iVScrlPos + hhTerm->iTermHite) <= 0)
         {
         hhTerm->fBackscrlLock = TRUE;
         termFillBk(hhTerm, hhTerm->iVScrlPos);
         }

      else
         {
         hhTerm->fBackscrlLock = FALSE;
         termFillBk(hhTerm, -hhTerm->iTermHite);
         }

      /* --- Vertical scroll bar --- */

      scrinf.cbSize= sizeof(scrinf);
      scrinf.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
      scrinf.nMin  = hhTerm->iVScrlMin;
      scrinf.nMax  = hhTerm->iVScrlMax + hhTerm->iTermHite - 1;
      scrinf.nPos  = hhTerm->iVScrlPos;
      scrinf.nPage = hhTerm->iTermHite;

      SetScrollInfo(hwnd, SB_VERT, &scrinf, TRUE);

      /* --- Horizontal scroll bar --- */

      i = hhTerm->iHScrlMax - hhTerm->iHScrlMin;
      scrinf.cbSize= sizeof(scrinf);
      scrinf.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
      scrinf.nMin  = hhTerm->iHScrlMin;
      scrinf.nMax  = hhTerm->iHScrlMax + HORZPAGESIZE - 1;
      scrinf.nPos  = hhTerm->iHScrlPos;
      scrinf.nPage = HORZPAGESIZE;

      SetScrollInfo(hwnd, SB_HORZ, &scrinf, TRUE);
      }

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_SETFOCUS
 *
 * DESCRIPTION:
 * Handler for WM_SETFOCUS message
 *
 * ARGUMENTS:
 * hwnd  - terminal window handle.
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_SETFOCUS(const HWND hwnd)
   {
   int i;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   hhTerm->fFocus = TRUE;

   // Use Multiplexer timer for cursor and blinking text.

   if (!hhTerm->hCursorTimer)
      {
      i = TimerCreate(sessQueryTimerMux(hhTerm->hSession),
            &hhTerm->hCursorTimer, (long)hhTerm->uBlinkRate,
               CursorTimerProc, (void *)hwnd);

      if (i != TIMER_OK)
         assert(FALSE);
      }

   ShowCursors(hhTerm);
   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_KILLFOCUS
 *
 * DESCRIPTION:
 * Handler for WM_KILLFOCUS message.
 *
 * ARGUMENTS:
 * hwnd  - terminal window handle.
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_KILLFOCUS(const HWND hwnd)
   {
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   hhTerm->fFocus = FALSE;

   if (hhTerm->hCursorTimer)
      TimerDestroy(&hhTerm->hCursorTimer);

   HideCursors(hhTerm);
   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_VSCROLL
 *
 * DESCRIPTION:
 * Message handler for terminal window's WM_VSCROLL
 *
 * ARGUMENTS:
 * hwnd     - terminal window handle
 * nCode    - scrollbar value
 * nPos     - scrollbox pos
 * hwndScrl - window handle of scrollbar
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_VSCROLL(HWND hwnd, int nCode, int nPos, HWND hwndScrl)
   {
   int i;
   int iScrlInc;
   RECT rc;
   SCROLLINFO scrinf;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   switch (nCode)
      {
      case SB_TOP:
         iScrlInc = -(hhTerm->iVScrlPos - hhTerm->iVScrlMin);
         break;

      case SB_BOTTOM:
         iScrlInc = hhTerm->iVScrlMax - hhTerm->iVScrlPos;
         break;

      case SB_LINEUP:
         iScrlInc = -1;
         break;

      case SB_LINEDOWN:
         iScrlInc = 1;
         break;

      case SB_PAGEUP:
         iScrlInc = min(-1, -hhTerm->iTermHite);
         break;

      case SB_PAGEDOWN:
         iScrlInc = max(1, hhTerm->iTermHite);
         break;

      case SB_THUMBTRACK:
      case SB_THUMBPOSITION:
         iScrlInc = nPos - hhTerm->iVScrlPos;
         break;

      default:
         iScrlInc = 0;
         break;
      }

   if ((iScrlInc = max(-(hhTerm->iVScrlPos - hhTerm->iVScrlMin),
         min(iScrlInc, hhTerm->iVScrlMax - hhTerm->iVScrlPos))) != 0)
      {
      HideCursors(hhTerm);
      hhTerm->iVScrlPos += iScrlInc;

      GetClientRect(hwnd, &rc);
      i = rc.bottom;
      rc.bottom = hhTerm->iTermHite * hhTerm->yChar;

      hhTerm->yBrushOrg = (hhTerm->yBrushOrg +
         (-hhTerm->yChar * iScrlInc)) % 8;

      ScrollWindow(hwnd, 0, -hhTerm->yChar * iScrlInc, 0, &rc);
      scrinf.cbSize = sizeof(scrinf);
      scrinf.fMask = SIF_DISABLENOSCROLL | SIF_POS;
      scrinf.nPos = hhTerm->iVScrlPos;
      SetScrollInfo(hwnd, SB_VERT, &scrinf, TRUE);

      hhTerm->fScrolled = TRUE;

      // Check to see if we scrolled entirely into backscroll
      // region.  If so, set fBackscrlLock for this view.

      if ((hhTerm->iVScrlPos + hhTerm->iTermHite) <= 0)
         {
         if (hhTerm->fBackscrlLock == FALSE)
            termFillBk(hhTerm, hhTerm->iVScrlPos);

         else
            termGetBkLines(hhTerm, iScrlInc, hhTerm->iVScrlPos, BKPOS_THUMBPOS);

         hhTerm->fBackscrlLock = TRUE;
         }

      else
         {
         if (hhTerm->fBackscrlLock == TRUE)
            termFillBk(hhTerm, -hhTerm->iTermHite);

         hhTerm->fBackscrlLock = FALSE;
         }

      // Make two seperate calls to WM_PAINT to optimize screen
      // updates in the case of a negative scroll operation.

      if (iScrlInc < 0)
         UpdateWindow(hwnd);

      // Fill-in fractional part of line at bottom of terminal
      // screen with appropriate color by invalidating that region.

      if (i > rc.bottom)
         {
         rc.top = rc.bottom;
         rc.bottom = i;
         InvalidateRect(hwnd, &rc, FALSE);
         }

      UpdateWindow(hwnd);
      ShowCursors(hhTerm);

      #if 0  // debugging stuff...
         {
         char ach[50];
         wsprintf(ach, "pos=%d min=%d max=%d", hhTerm->iVScrlPos,
            hhTerm->iVScrlMin, hhTerm->iVScrlMax);
         SetWindowText(sessQueryHwndStatusbar(hhTerm->hSession), ach);
         }
      #endif
      }

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_HSCROLL
 *
 * DESCRIPTION:
 * Message handler for terminal window's WM_HSCROLL
 *
 * ARGUMENTS:
 * hwnd     - terminal window handle
 * nCode    - scrollbar value
 * nPos     - scrollbox pos
 * hwndScrl - window handle of scrollbar
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_HSCROLL(HWND hwnd, int nCode, int nPos, HWND hwndScrl)
   {
   int i, j, k;
   int iScrlInc;
   RECT rc;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   switch (nCode)
      {
      case SB_LINEUP:
         iScrlInc = -1;
         break;

      case SB_LINEDOWN:
         iScrlInc = 1;
         break;

      case SB_PAGEUP:
         iScrlInc = -HORZPAGESIZE;
         break;

      case SB_PAGEDOWN:
         iScrlInc = HORZPAGESIZE;
         break;

      case SB_THUMBTRACK:
      case SB_THUMBPOSITION:
         iScrlInc = nPos - hhTerm->iHScrlPos;
         break;

      default:
         iScrlInc = 0;
         break;
      }

   if ((iScrlInc = max(-hhTerm->iHScrlPos,
         min(iScrlInc, hhTerm->iHScrlMax - hhTerm->iHScrlPos))) != 0)
      {
      HideCursors(hhTerm);
      i = -hhTerm->xChar * iScrlInc;
      j = 0;

      hhTerm->iHScrlPos += iScrlInc;
      GetClientRect(hwnd, &rc);
      rc.left += hhTerm->xIndent;

      // We have to adjust for the bezel when it is visible
      if (hhTerm->xBezel &&
            (hhTerm->iHScrlPos == 0 ||
            (hhTerm->iHScrlPos - iScrlInc) == 0)   ||
             hhTerm->iHScrlPos == hhTerm->iHScrlMax ||
            (hhTerm->iHScrlPos - iScrlInc == hhTerm->iHScrlMax))
         {
         k = hhTerm->xBezel - hhTerm->xChar;
         i += (iScrlInc > 0) ? -k : k;
         j = 1;   // set for test below
         }

      ScrollWindow(hwnd, i, 0, 0, &rc);
      SetScrollPos(hwnd, SB_HORZ, hhTerm->iHScrlPos, TRUE);


#if defined(FAR_EAST)
   InvalidateRect(hwnd, NULL, FALSE);
#endif

      hhTerm->fScrolled = TRUE;

      if (j == 1)
         {
         GetUpdateRect(hwnd, &rc, FALSE);
         rc.left = 0;   // makes sure bezel is drawn.
         InvalidateRect(hwnd, &rc, FALSE);
         }

      UpdateWindow(hwnd);
      ShowCursors(hhTerm);
      }

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_CHAR
 *
 * DESCRIPTION:
 * Processes the WM_CHAR and WM_KEYDOWN messages for termproc.
 *
 * ARGUMENTS:
 * hwnd  - terminal window handle
 * message - WM_CHAR or WM_KEYDOWN
 * wPar  - wParam
 * lPar  - lParam
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_CHAR(const HWND hwnd, const UINT message,
                  const WPARAM wPar, const LPARAM lPar)
   {
   MSG msg;
   KEY_T Key;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   if (GetKeyState(VK_LBUTTON) < 0)
      return;

    //DbgOutStr("TP_WM_CHAR 0x%x 0x%x 0x%lx\r\n", message, wPar, lPar, 0,0);

   msg.hwnd = hwnd;
   msg.message = message;
   msg.wParam  = wPar;
   msg.lParam  = lPar;

   Key = TranslateToKey(&msg);

   if (!termTranslateKey(hhTerm, hwnd, Key))
      {
      CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), &Key, 1, CLOOP_KEYS);
      LinkCursors(hhTerm);
      }

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_IME_CHAR
 *
 * DESCRIPTION:
 * Processes the WM_IME_CHAR message for FE version of WACKER
 *
 * ARGUMENTS:
 * hwnd  - terminal window handle
 * message - WM_IME_CHAR
 * wPar  - wParam
 * lPar  - lParam
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_IME_CHAR(const HWND hwnd, const UINT message,
                     const WPARAM wPar, const LPARAM lPar)
   {
   KEY_T ktCode1;
   KEY_T ktCode2;
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    //DbgOutStr("TP_WM_IME_CHAR 0x%x 0x%x 0x%lx\r\n", message, wPar, lPar, 0,0);

   /*
    * Later on, we may decide to create or modify something like the stuff
    * that goes on in TP_WM_CHAR, but until we get a better understanding of
    * how to process this stuff, just do it right here, in line, in all of
    * its ugly experimental foolishness.
    */
   ktCode1 = (KEY_T)(wPar & 0xFF);         /* Lower eight bits */
   ktCode2 = (KEY_T)((wPar >> 8) & 0xFF); /* Upper eight bits */

   /*
    * The documentation says that the lower eight bits are the first char,
    * but I am not too sure about that.  It looks like the upper eight bits
    * is the lead byte and the lower eight bits is not.  Keep an eye on this.
    */

   //mpt:2-7-98 apparently, the korean ime can send us single byte characters
   //           with 'null' as the other character, for some reason most host
   //           systems don't like it when we send that null out the port.
   if (ktCode2 != 0)
      CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), &ktCode2, 1, CLOOP_KEYS);

   if (ktCode1 != 0)
      CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), &ktCode1, 1, CLOOP_KEYS);

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_TERM_KEY
 *
 * DESCRIPTION:
 * Term keys are keys we know are going to be translated by the
 * session macro or emulator sequence
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void TP_WM_TERM_KEY(const HWND hwnd, KEY_T Key)
   {
   // Until we have a real macro key expander...
   const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   if (GetKeyState(VK_LBUTTON) < 0)
      return;

   if ( kabExpandMacroKey(hhTerm->hSession, Key) == 0)
        {
      CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), &Key, 1, CLOOP_KEYS);
        }

   LinkCursors(hhTerm);
   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_TERM_TRACK
 *
 * DESCRIPTION:
 * Causes terminal to track to the cursor position.  If wPar is
 * 0, then we track only if we are not backscroll-locked and not
 * Mark-locked.  If wPar is not 0, then we force tracking,
 * unmarking and unlocking the backscroll if necessary
 *
 * ARGUMENTS:
 * hhTerm   - private term handle
 * fForce   - force tracking no matter what.
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_TERM_TRACK(const HHTERM hhTerm, const int fForce)
   {
   int i, j;

   if ((hhTerm->fCursorTracking == 0 || hhTerm->fBackscrlLock != 0)
         && fForce == 0)
      {
      return;
      }

   j = 0;

   // First track vertically

   if (hhTerm->ptHstCur.y < hhTerm->iVScrlPos)
      i = hhTerm->ptHstCur.y;

   else if ((hhTerm->ptHstCur.y - hhTerm->iTermHite + 2) > hhTerm->iVScrlPos)
      i = hhTerm->ptHstCur.y - hhTerm->iTermHite + 2;

   else
      i = hhTerm->iVScrlPos;

   if (i != hhTerm->iVScrlPos)
      {
      // If we have enough room to display the entire terminal
      // then go go to iVScrlMax

      if (hhTerm->iTermHite > hhTerm->iRows)
         i = hhTerm->iVScrlMax;

      SendMessage(hhTerm->hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, i), 0);
      j = 1;
      }

   // Now check for horizontal placement and adjust to make cursor
   // visible.

   if (hhTerm->ptHstCur.x < hhTerm->iHScrlPos)
      {
      i = hhTerm->ptHstCur.x;
      }

   else if (hhTerm->ptHstCur.x >= hhTerm->iHScrlPos +
                     hhTerm->iCols - hhTerm->iHScrlMax)
      {
      i = hhTerm->ptHstCur.x - (hhTerm->iCols - hhTerm->iHScrlMax) + 5;
      }

   else
      {
      i = hhTerm->iHScrlPos;
      }

   if (i != hhTerm->iHScrlPos)
      {
      SendMessage(hhTerm->hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, i), 0);
      j = 1;
      }

   if (j)
      LinkCursors(hhTerm);

   return;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_EMU_SETTINGS
 *
 * DESCRIPTION:
 * Something important in the emulator has changed (like loading a new
 * emulator or changing the number of rows or columns).
 *
 * ARGUMENTS:
 * hhTerm   - private terminal handle.
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_EMU_SETTINGS(const HHTERM hhTerm)
   {
   int iRows, iCols;
   BOOL fChange = FALSE;
   int iCurType;
   STEMUSET stEmuUserSettings;

   const HEMU hEmu = sessQueryEmuHdl(hhTerm->hSession);

   /* --- Check rows and columns --- */

   emuQueryRowsCols(hEmu, &iRows, &iCols);

   if (iRows != hhTerm->iRows || iCols != hhTerm->iCols)
      {
      hhTerm->iRows = iRows;
      hhTerm->iCols = iCols;
      fChange = TRUE;
      }

   /* --- Query other emulator settings --- */

   iCurType = emuQueryCursorType(hEmu);

   emuQuerySettings(hEmu, &stEmuUserSettings);
   hhTerm->fBlink = stEmuUserSettings.fCursorBlink;

   /* --- Check cursor type --- */

   if (iCurType != hhTerm->iCurType)
      {
      fChange = TRUE;
      hhTerm->iCurType = iCurType;

      switch (hhTerm->iCurType)
         {
      case EMU_CURSOR_LINE:
      default:
         hhTerm->iHstCurSiz = GetSystemMetrics(SM_CYBORDER) * 2;
         break;

      case EMU_CURSOR_BLOCK:
         hhTerm->iHstCurSiz = hhTerm->yChar;
         break;

      case EMU_CURSOR_NONE:
         hhTerm->iHstCurSiz = 0;
         break;
         }
      }

   if (fChange)
      RefreshTermWindow(hhTerm->hwnd);
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_TERM_LOAD_SETTINGS
 *
 * DESCRIPTION:
 * Loads terminal settings from session file.
 *
 * ARGUMENTS:
 * hhTerm   - private terminal handle
 *
 * RETURNS:
 * 0=OK
 *
 */
static int TP_WM_TERM_LOAD_SETTINGS(const HHTERM hhTerm)
   {
   LONG lSize;

   if (hhTerm == 0)
      {
      assert(FALSE);
      return 1;
      }

   lSize = sizeof(hhTerm->lfSys);

   if (sfGetSessionItem(sessQuerySysFileHdl(hhTerm->hSession),
         SFID_TERM_LOGFONT, &lSize, &hhTerm->lfSys) == 0)
      {
      termSetFont(hhTerm, &hhTerm->lfSys);
      RefreshTermWindow(hhTerm->hwnd);
      }

   return 0;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_TERM_SAVE_SETTINGS
 *
 * DESCRIPTION:
 * Saves terminal settings to session file.
 *
 * ARGUMENTS:
 * hhTerm   - private terminal handle
 *
 * RETURNS:
 * 0=OK
 *
 */
static int TP_WM_TERM_SAVE_SETTINGS(const HHTERM hhTerm)
   {
   if (hhTerm == 0)
      {
      assert(FALSE);
      return 1;
      }

   if (memcmp(&hhTerm->lf, &hhTerm->lfSys, sizeof(hhTerm->lf)))
      {
      sfPutSessionItem(sessQuerySysFileHdl(hhTerm->hSession),
         SFID_TERM_LOGFONT, sizeof(hhTerm->lf), &hhTerm->lf);
      }

   return 0;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 * TP_WM_TERM_FORCE_WMSIZE
 *
 * DESCRIPTION:
 * Forces the terminal through its resize code to get things properly
 * updated.  Used only when session is first initialized.
 *
 * ARGUMENTS:
 * hhTerm   - private terminal handle.
 *
 * RETURNS:
 * void
 *
 */
static void TP_WM_TERM_FORCE_WMSIZE(const HHTERM hhTerm)
   {
   RECT rc;

   if (!IsWindow(hhTerm->hwnd))
      return;

   hhTerm->iVScrlPos = hhTerm->iVScrlMax;
   hhTerm->iHScrlPos = 0;

   GetClientRect(hhTerm->hwnd, &rc);
   termGetUpdate(hhTerm, FALSE);
   TP_WM_SIZE(hhTerm->hwnd, 0, rc.right, rc.bottom);
   InvalidateRect(hhTerm->hwnd, 0, FALSE);
   return;
   }

//******************************************************************************
// Method:
//    kabExpandMacroKey
//
// Description:
//    Determines if the key is a macro key and if so expands the macro and send
//    the keys to the CLOOP
//
// Arguments:
//    hSession - Session handle
//    aKey     - The key to be expanded
//
// Returns:
//    non zero - if the key has been expanded, zero if the key is not a macro
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/10/1998
//
//

static int kabExpandMacroKey( const HSESSION hSession, KEY_T aKey )
    {
    int lReturn = 0;

#if defined INCL_KEY_MACROS
    keyMacro lKeyMacro;
    int lKeyIndex = -1;
    int lIndex    = 0;

    lKeyMacro.keyName = aKey;
    lKeyIndex = keysFindMacro( &lKeyMacro );

    if ( lKeyIndex >= 0 )
        {
        keysGetMacro( lKeyIndex, &lKeyMacro );

        CLoopSend( sessQueryCLoopHdl(hSession), &lKeyMacro.keyMacro,
                   lKeyMacro.macroLen, CLOOP_KEYS );

        lReturn = 1;
        }
#endif

    return lReturn;
    }
