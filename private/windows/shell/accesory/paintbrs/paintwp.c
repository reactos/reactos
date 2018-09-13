/****************************Module*Header******************************\
* Module Name: paintwp.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#define NOCOLOR
#define NOCOMM
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOKANJI
#define NOKEYSTATES
#define NOLSTRING
#define NOMB
#define NOMDI
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOOPENFILE
#define NOPROFILER
#define NORESOURCE
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSMETRICS
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINSTYLES
// Added by pat 6-21-91
#ifndef WIN32
#define WIN31
#endif

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "pbserver.h"

#ifdef DBCS_IME        //  added by Hiraisi
#include <ime.h>
HANDLE hString = NULL;        // for chars from IME.
static BOOL bOpenConvert = FALSE;
void HideIMEWindow( HWND );
#endif

#ifdef PENWIN
#include <penwin.h>
extern BOOL fIPExists;
extern VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL);
extern BOOL PUBLIC TapInText( POINT );
extern BOOL (FAR PASCAL *lpfnTPtoDP)(LPPOINT, int);
extern BOOL (FAR PASCAL *lpfnIsPenEvent)( WORD, LONG);
#endif

extern int UpdateCount;
extern HWND pbrushWnd[];
extern POINT csrPt;
extern DPPROC DrawProc;
extern struct csstat CursorStat;
extern int cursTool;
extern LPTSTR DrawCursor;
extern BOOL inMagnify, TerminateKill, imageFlag;
extern int paintWid, paintHgt;
extern int imageWid, imageHgt;
extern RECT imageView;
extern BOOL mouseFlag;
extern HPALETTE hPalette;
extern BOOL drawing, moving;
extern HWND mouseWnd;

BOOL bJustActivated = FALSE;

#if !defined(UNICODE) && defined(DBCS_IME)      //  added by Hiraisi
WORD NEAR PASCAL EatOneCharacter(HWND);
/*
 * routine to retrieve WM_CHAR from the message queue associated with hwnd.
 * this is called by EatString.
 */
WORD NEAR PASCAL EatOneCharacter(hwnd)
register HWND hwnd;
{
    MSG msg;
    register int i = 10;

    while(!PeekMessage((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
        if (--i == 0)
            return -1;
        Yield();
    }
    return(msg.wParam & 0xFF);
}

BOOL EatString(HWND,LPSTR,WORD);
/*
 * The purpose of this function is to eat
 * all WM_CHARs between IR_STRINGSTART and IR_STRINGEND and to build a
 * string block.
 */
BOOL EatString(hwnd, lpSp, cchLen)
register HWND   hwnd;
LPSTR lpSp;
WORD cchLen;
{
    MSG msg;
    int i = 10;
    int w;

    *lpSp = '\0';
    if (cchLen < 4)
        return NULL;    // not enough
    cchLen -= 2;

    while (i--)
    {
        // We don't want to peek WM_PAINT.
        while (PeekMessage (&msg,hwnd,WM_CHAR,WM_IME_REPORT,PM_REMOVE))
        {
            i = 10;
            switch (msg.message){
            case WM_CHAR:
                *lpSp++ = (BYTE)msg.wParam;
                cchLen--;
                if (IsDBCSLeadByte((BYTE)msg.wParam))
                {
                    if ((w = EatOneCharacter(hwnd)) == -1) {
                        /* Bad DBCS sequence - abort */
                        lpSp--;
                        goto WillBeDone;
                    }
                    *lpSp++ = (BYTE)w;
                    cchLen--;
                }
                if (cchLen <= 0)
                    goto WillBeDone;   // buffer exhausted
                break;

            case WM_IME_REPORT:
                if (msg.wParam == IR_STRINGEND)
                {
                    if (cchLen <= 0)
                        goto WillBeDone; // no more room to stuff
                    if ((w = EatOneCharacter(hwnd)) == -1)
                        goto WillBeDone;
                    *lpSp++ = (BYTE)w;
                    if (IsDBCSLeadByte((BYTE)w))
                    {
                        if ((w = EatOneCharacter(hwnd)) == -1) {
                            /* Bad DBCS sequence - abort */
                            lpSp--;
                            goto WillBeDone;
                        }
                        *lpSp++ = (BYTE)w;
                    }
                    goto WillBeDone;
                }

                /* Fall through */

            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
            }
        }
    }
    /* We don't get WM_IME_REPORT + IR_STRINGEND
     * But received string will be OK
     */

WillBeDone:

    *lpSp = '\0';

    return TRUE;
}
#endif        //  JAPAN added by Hiraisi


void SetNewproc(DPPROC newproc)
{
   POINT   oldPt;
   HWND    hWnd;
   HCURSOR oldcsr;

   oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
   hWnd = pbrushWnd[PAINTid];
   oldPt = csrPt;

   /* terminate previous function */
   SendMessage(hWnd, WM_TERMINATE, 0, 0L);

   /* set the drawing procedure */
   DrawProc = newproc;
   if (CursorStat.noted)
       CursorStat.noted = FALSE;
   cursTool = LCUNDOtool;
   DrawCursor = NULL;
   SetCursor(oldcsr);

   /* turn the cursor off (to get rid of old cursor) */
   SendMessage(hWnd, WM_HIDECURSOR, 0, MAKELONG(oldPt.x, oldPt.y));

   /* turn the cursor back on (to show new cursor) */
   SendMessage(hWnd, WM_MOUSEMOVE, 0, MAKELONG(oldPt.x, oldPt.y));
}

long FAR PASCAL PaintWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int             i, j;
   PAINTSTRUCT     ps;
   LPCREATESTRUCT  cs;
   POINT           oldPt;
   HCURSOR         oldcsr;
   HWND            hAWnd;
   HDC             paintDC;
   BOOL            IsVis, bFlag;

   if (message == WM_KEYDOWN || message == WM_KEYUP)
       SendMessage(pbrushWnd[PARENTid], message, wParam, lParam);

   if (inMagnify)
       return ZoomInWP(hWnd, message, wParam, lParam);

   switch (message) {
       case WM_ACTIVATE:
           if (GET_WM_ACTIVATE_STATE(wParam,lParam) == 2)
               SetFocus(hWnd);     /* activated by mouse click */
           else
               return DefWindowProc(hWnd, message, wParam, lParam);
           break;

       case WM_KILLFOCUS:      /* other app selected */
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           if ((!CursorStat.captured) && TerminateKill) {
               SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
               UpdatImg();
           }
           SetCursor(oldcsr);
           break;

               /* setup image variables */
       case WM_CREATE:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           cs = (LPCREATESTRUCT) lParam;
           paintWid = cs->cx;
           paintHgt = cs->cy;
           CalcView();
           SetCursor(oldcsr);
           break;

               /* resize the paint window */
      case WM_SIZE:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           paintWid = LOWORD(lParam);
           paintHgt = HIWORD(lParam);
           CalcView();
// C8.17 x86 compilier bug work around
#ifdef OLDWAY
           i = imageWid - paintWid;
           j = imageHgt - paintHgt;
#else
           i = imageWid - LOWORD(lParam);
           j = imageHgt - HIWORD(lParam);
#endif
           i = max(0,i);
           j = max(0,j);

           SetScrollRange(hWnd, SB_HORZ, 0, i, FALSE);
           SetScrollRange(hWnd, SB_VERT, 0, j, FALSE);
/* EDH */
           SetScrollPos(hWnd, SB_HORZ, imageView.left, TRUE);
           SetScrollPos(hWnd, SB_VERT, imageView.top, TRUE);

           SetCursor(oldcsr);
           break;

       case WM_MOVE:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           if (mouseFlag) {
               SendMessage(pbrushWnd[PARENTid], WM_COMMAND, MISCmousePos, 0L);
               SendMessage(pbrushWnd[PARENTid], WM_COMMAND, MISCmousePos, 0L);
           }
           SetCursor(oldcsr);
           break;

               /* vertical scroll bar action */
       case WM_VSCROLL:
       case WM_HSCROLL:
           bFlag = (GET_WM_HSCROLL_CODE(wParam,lParam) == SB_BOTTOM ||
                    GET_WM_HSCROLL_CODE(wParam,lParam) == SB_PAGEDOWN ||
                    GET_WM_HSCROLL_CODE(wParam,lParam) == SB_PAGEUP ||
                    GET_WM_HSCROLL_CODE(wParam,lParam) == SB_THUMBPOSITION ||
                    GET_WM_HSCROLL_CODE(wParam,lParam) == SB_TOP);
           if (bFlag)
               oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           ScrolImg(hWnd, message, wParam, lParam);
           if (bFlag)
               SetCursor(oldcsr);
           break;

               /* paint the paint window */
       case WM_PAINT:
           if (fInvisible)
           {
               BeginPaint(hWnd, &ps);
               EndPaint(hWnd, &ps);
               break;
           }

           /* If we called UpdateColors more than once the screen has probably
               become too degrated */
           if (UpdateCount > 1) {
               BeginPaint(hWnd, &ps);
               EndPaint(hWnd, &ps);
               UpdateCount = 0;
               InvalidateRect(hWnd, (LPRECT) NULL, FALSE);
               break;
           }

           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           BeginPaint(hWnd, &ps);

           if (hPalette) {
               SelectPalette(ps.hdc, hPalette, FALSE);
               RealizePalette(ps.hdc);
           }

           i = ps.rcPaint.left;
           j = ps.rcPaint.top;

           BitBlt(ps.hdc, i, j,
                   1 + ps.rcPaint.right  - i, 1 + ps.rcPaint.bottom - j,
                   hdcWork, i + imageView.left, j + imageView.top, SRCCOPY);
           (*DrawProc)(hWnd, message, wParam, (LONG)(LPPAINTSTRUCT) &ps);

           EndPaint(hWnd, &ps);
           SetCursor(oldcsr);
           break;

      case WM_TERMINATE:
      case WM_HIDECURSOR:
           CursorStat.noted = FALSE;
           if (CursorStat.captured) {
               ReleaseCapture();
               CursorStat.captured = FALSE;
               SetCursor(LoadCursor(NULL, IDC_ARROW));
           }
           SETCLASSCURSOR(hWnd, LoadCursor(NULL, IDC_ARROW));
           (*DrawProc)(hWnd, message, wParam, lParam);
           break;

      case WM_MOUSEMOVE:
           if ((hAWnd = GetActiveWindow()) != pbrushWnd[PARENTid]
               && hAWnd != pbrushWnd[PAINTid])
               break;

           /* drawing - drawing an outline e.g. rectangle
               moving - moving a cut/paste box on screen */
           if (!(drawing || moving)) {
               LONG2POINT(lParam,oldPt);

               paintDC = GetDisplayDC(hWnd);
               IsVis = PtVisible(paintDC, oldPt.x, oldPt.y);
               ReleaseDC(hWnd, paintDC);

               if (!IsVis) {
                   SendMessage(hWnd, WM_HIDECURSOR, 0, 0L);
                   break;
               } else if (!CursorStat.noted) {
                   CursorStat.noted = TRUE;
                   PbSetCursor(DrawCursor);
                   if (!(CursorStat.allowed || CursorStat.captured)) {
                        DB_OUT("Set in Paintwp\n");
                        SetCapture(hWnd);
                        CursorStat.captured = TRUE;
                   }
                   SendMessage(hWnd, WM_SHOWCURSOR, wParam, lParam);
               }
           }

           if (mouseFlag)
               PostMessage(mouseWnd, WM_MOUSEPOS, 0, lParam);
           (*DrawProc)(hWnd, message, wParam, lParam);
           break;

#if !defined(UNICODE) && defined(DBCS_IME)      //  added by Hiraisi
       case WM_IME_REPORT:
       {
           LPSTR lpString, lpP;
           LONG lGetString;

           switch( wParam ){
           case IR_STRING:
               HideIMEWindow( hWnd );
//               if( lpP = GlobalLock((HANDLE)LOWORD(lParam)) ){
               if( lpP = GlobalLock((HANDLE)(lParam)) ){
                   hString=GlobalAlloc(GMEM_MOVEABLE,(DWORD)ByteCountOf(lstrlen(lpP)+1));
                   if( hString ){
                       lpString = GlobalLock( hString );
                       lstrcpy( lpString, lpP );       // Save chars
                       GlobalUnlock( hString );
                       lGetString = 1L;
                       if( !bOpenConvert ){
                       /*
                        *  We need to update pbrushWnd[PAINTid] before we write
                        * string sent from IME. The reason is that the window
                        * has not been updated yet though IME's conversion
                        * window has already closed by this time.
                        *  But we don't call UpdateWindow because IME's window
                        * was somtimes out of pbrushWnd[PAINTid].
                       */
                           SendMessage(pbrushWnd[PAINTid], WM_PAINT, 0, 0L );
               SendMessage(pbrushWnd[PAINTid], WM_IME_CHAR, hString, 0 );
               hString = GlobalFree( hString );
                       }
                   }
                   else{
                       lGetString = 0L;
                   }
//                   GlobalUnlock((HANDLE)LOWORD(lParam));
                   GlobalUnlock((HANDLE)(lParam));
                   return lGetString;
               }
               break;
           case IR_STRINGSTART:
               if( (hString = GlobalAlloc(GMEM_MOVEABLE, 512L)) ){
                   lpString = GlobalLock(hString);
                   //  not pbrushWnd[PAINTid]
                   if( !EatString(pbrushWnd[PARENTid], lpString, 512) ){
                       GlobalUnlock(hString);
                       GlobalFree(hString);
                       break;
                   }
                   GlobalUnlock(hString);
               }
               break;

           /*
            *  We won't need under case sentences if all IMEs send IR_STRING
            * after IR_CLOSECONVERT.
           */
           case IR_OPENCONVERT:
           case IR_CHANGECONVERT:
           case IR_FULLCONVERT:
               bOpenConvert = TRUE;
               break;
           case IR_CLOSECONVERT:
               bOpenConvert = FALSE;
               if( hString )
               {
                   BOOL CheckIMEConvWnd( void );   /* included in textdp.c */

                        if ( CheckIMEConvWnd( ) ) {
                       SendMessage(pbrushWnd[PAINTid], WM_PAINT, 0, 0L );
                               SendMessage(pbrushWnd[PAINTid], WM_IME_CHAR, hString, 0 );
                               hString = GlobalFree( hString );
                                        }
               }
               break;
           default:
               break;
           }
       }
           break;
#endif    // !UNICODE && DBCS_IME

#ifdef KOREA
       case WM_INTERIM:
#endif
       case WM_CHAR:
           if ((TCHAR) wParam == BS && DrawProc != Text2DP
                                   && DrawProc != LcUndoDP)
               SetNewproc(LcUndoDP);
           else
               (*DrawProc)(hWnd,message,wParam,lParam);
           break;

      case WM_LBUTTONDOWN:
           if (bJustActivated) {
               bJustActivated = FALSE;
               break;
           }

           if (!CursorStat.noted)      /* such as a menu being down */
               SendMessage(pbrushWnd[PAINTid], WM_MOUSEMOVE, wParam, lParam);

           if (!CursorStat.captured) {
               SetCapture(hWnd);
               DB_OUT("Set in paintwp\n");
               CursorStat.captured = TRUE;
            }

           LONG2POINT(lParam,oldPt);
           if (((theTool != PICKtool) && (theTool != SCISSORStool)) ||
                 PtInRect(&pickRect, oldPt))
               //  added by Hiraisi (BUG#2867/WIN31)
               if( cursTool != ZOOMINtool )
                   imageFlag = TRUE;

#ifdef PENWIN
           if(!lpfnRegisterPenApp)  // if not pen system, update everything
              UpdFlag(TRUE);
           else                     // if pen sys, update only if not pen event
              if((!(*lpfnIsPenEvent)( message, GetMessageExtraInfo())) || (DrawProc != Text2DP ))
#endif
                 UpdFlag(TRUE);

           /* **** FALL THROUGH TO WM_MBUTTONDOWN **** */

      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
           if (bJustActivated) {
               bJustActivated = FALSE;
               break;
           }

           /* such as a menu being down */
           if ((message != WM_LBUTTONDOWN) && !CursorStat.noted)
               SendMessage(pbrushWnd[PAINTid], WM_MOUSEMOVE, wParam, lParam);

           (*DrawProc)(hWnd, message, wParam, lParam);
            break;

       case WM_LBUTTONUP:
           if (CursorStat.captured
                                         && CursorStat.allowed) {

               DB_OUT("ReleaseCapture in paintwp\n");
               ReleaseCapture();
               CursorStat.captured = FALSE;
           }

           /* **** FALL THROUGH TO DrawProc **** */

       case WM_RBUTTONUP:
       case WM_LBUTTONDBLCLK:
       case WM_RBUTTONDBLCLK:
       case WM_SHOWCURSOR:
       case WM_CHANGEFONT:
       case WM_ZOOMUNDO:
       case WM_ZOOMACCEPT:
       case WM_SCROLLINIT:
       case WM_SCROLLDONE:
       case WM_SCROLLVIEW:
       case WM_PICKFLIPH:
       case WM_PICKFLIPV:
       case WM_PICKINVERT:
       case WM_PICKSG:
       case WM_PICKTILT:
       case WM_PICKCLEAR:
       case WM_CUT:
       case WM_COPY:
       case WM_PASTE:
       case WM_COPYTO:
       case WM_PASTEFROM:
       case WM_WHOLE:
       case WM_OUTLINE:
#if !defined(UNICODE) && defined(DBCS_IME)      //  added by Hiraisi  26 Aug. 1992
       case WM_IME_CHAR:
#endif
#ifdef JAPAN //KKBUGFIX      // added by Hiraisi  04 Sep. 1992 (in Japan)
       case WM_RESETCARET:
#endif
           (*DrawProc)(hWnd, message, wParam, lParam);
           break;
#ifdef PENWIN
    case WM_RCRESULT:
                {
                        LPRCRESULT lpr = (LPRCRESULT)lParam;
                        POINT newPt, pt;
                           if (lpr->wResultsType&RCRT_GESTURE)
                                   {
                                   switch ( LOWORD(*(lpr->lpsyv)))
                                           {
                                           case LOWORD( SYV_CORRECT ):
                                                   (*DrawProc)(hWnd, WM_CORRECTTEXT, wParam, lParam);
                                                   return 1;
                                                   break;
                                           case LOWORD( SYV_BACKSPACE ):
                     SendMessage(hWnd, WM_CHAR, 0x08, 0L);
                     return 1;
                     break;
                                           case LOWORD( SYV_RETURN ):
                     SendMessage(hWnd, WM_CHAR, 0x0D, 0L);
                     return 1;
                     break;
                                           case LOWORD( SYV_SPACE ):
                     SendMessage(hWnd, WM_CHAR, 0x20, 0L);
                     return 1;
                     break;
                                           default:
                                                   return 1;
                                                   break;

                                           }
                                   }
                           if (lpr->cSyv != 0 && !fIPExists) // is this the right way to check for GOOD chars??
                                   {
                                   pt.y = (lpr->rectBoundInk.top + lpr->rectBoundInk.bottom)/2;
                                   pt.x = lpr->rectBoundInk.left;
               UpdFlag(TRUE);
                                   // NOTE: rectBoundInk data is in client coordinates (not tablet)
                                   SendMessage( hWnd, WM_LBUTTONDOWN, NULL, MAKELONG( pt.x, pt.y ));
                                   }
                }
#endif // PENWIN

       case WM_COMMAND:
           /* lParam = 0 means message from menu; 1 means from accelerator */
           if (!GET_WM_COMMAND_HWND(wParam,lParam))
               MenuCmd(hWnd, GET_WM_COMMAND_ID(wParam,lParam));
           break;

       default:
           return DefWindowProc(hWnd, message, wParam, lParam);
           break;
   }

   return 0L;
}

