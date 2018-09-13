
/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/
/*--------------------------------------------------------------------------*/

#include <windows.h>
#include <port1632.h>
#include "hack.h"

/*--------------------------------------------------------------------------*/

#define SZCODE  char _based(_segname("_CODE"))
#define TIMER_ID        1
#define MS_SCROLLTIME   150

/*--------------------------------------------------------------------------*/

static  HWND    hwndParent;
static  RECT    rcUp;
static  RECT    rcDown;
static  SZCODE  szArrowClass[] = "cpArrow";

static  UINT    uScroll;             /* gee, thanks for the helpful comment.  Laurie. */
static  UINT    uTimer;
static  BOOL    fKeyDown;

/*
 * fDownButton
 *
 * TRUE if we are dealing with the 'down arrow'.  FALSE if we are dealing
 * with the 'up arrow'.
 */
typedef enum tagArrowDirection {
        enumArrowUp,
        enumArrowDown
}       ARROWDIRECTION;
static  ARROWDIRECTION  ArrowType;

/*
 * fButton
 *
 * TRUE if the user pressed the left button down on the arrow window,
 * as long as cursor is still over that window and the left button
 * remains down.
 */
static  BOOL    fButton;

/*
 * fRealButton
 *
 * TRUE if the user pressed the left button down on the arrow window,
 * regardless of whether or not the cursor is still over the window, as
 * long as the left button remains down.
 */
static  BOOL    fRealButton;

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR KeyDown(
        HWND    hwnd,
        UINT    uKey)
{
        DWORD   dCoordinates;

        if (!fKeyDown && ((uKey == VK_DOWN) || (uKey == VK_UP))) {
                fKeyDown = TRUE;
                hwndParent = GetParent(hwnd);
                GetClientRect(hwnd, &rcUp);
                rcUp.bottom = (rcUp.top + rcUp.bottom) / 2 - 1;
                rcDown.top = rcUp.bottom + 1;
                if (uKey == VK_DOWN)
                        dCoordinates = MAKELONG(0, rcDown.top);
                else
                        dCoordinates = MAKELONG(0, rcUp.top);
                SendMessage(hwnd, WM_LBUTTONDOWN, (WPARAM)0, (LPARAM)dCoordinates);
        }
}

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR KeyUp(
        HWND    hwnd,
        UINT    uKey)
{
        if (fKeyDown && ((uKey == VK_DOWN) || (uKey == VK_UP))) {
                fKeyDown = FALSE;
                SendMessage(hwnd, WM_LBUTTONUP, (WPARAM)0, (LPARAM)0);
        }
}

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR LButtonDown(
        HWND    hwnd,
        int     iCoord)
{
        if (!fRealButton) {
                fButton = TRUE;
                fRealButton = TRUE;
                SetCapture(hwnd);
                hwndParent = GetParent(hwnd);
                GetClientRect(hwnd, &rcUp);
                CopyRect(&rcDown, &rcUp);
                rcUp.bottom = (rcUp.top + rcUp.bottom) / 2 - 1;
                rcDown.top = rcUp.bottom + 1;
                uScroll = (iCoord >= rcDown.top) ? SB_LINEDOWN : SB_LINEUP;
                ArrowType = (uScroll == SB_LINEDOWN) ? enumArrowDown : enumArrowUp;
#if defined(WIN16)
                SendMessage(hwndParent, WM_VSCROLL, (WPARAM)uScroll, MAKELPARAM(GetWindowWord(hwnd, GWW_ID), hwnd));
#else

                /* The NT version of WM_VSCROLL wants the scroll position.
                   (Actually the book is vague, maybe it only wants it for
                   SB_THUMBPOSITION and SB_THUMBTRACK)
                   However we may be able to fudge it anyway.
                   So long as the message is sent to a WNDPROC ported from
                   DOS it will not be looking for the scroll position in
                   other cases.  Neither book (DOS or NT) mentions stuffing
                   the ID in,  but the line above does so.
                   Fortunately, nobody looks at it!
                   Whenever it stuffs something into the LOWORD(lParam)
                   on DOS we use HIWORD(wParam) on NT
                */
                SendMessage( hwndParent
                           , WM_VSCROLL
                           , (WPARAM)uScroll
                           , (LPARAM)hwnd
                           );
#endif //WIN16
                uTimer = SetTimer(hwnd, TIMER_ID, MS_SCROLLTIME, NULL);
                if (ArrowType == enumArrowDown)
                        InvalidateRect(hwnd, &rcDown, TRUE);
                else
                        InvalidateRect(hwnd, &rcUp, TRUE);
        }
}

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR MouseMove(
        HWND    hwnd,
        POINT   pt)
{
        // if they didn't left button down on us originally, ignore it;
        if (fRealButton) {
                BOOL    fGray;

                fGray = (((ArrowType == enumArrowDown) && PtInRect(&rcDown, pt)) || ((ArrowType == enumArrowUp) && PtInRect(&rcUp, pt)));
                // either not over the arrow window anymore, just came on top of window;
                if ((fButton && !fGray) || (!fButton && fGray)) {
                        fButton = !fButton;
                        InvalidateRect(hwnd, (ArrowType == enumArrowDown) ? &rcDown : &rcUp, TRUE);
                }
        }
}

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR LButtonUp(
        HWND    hwnd)
{
        if (fButton) {
                ReleaseCapture();
#if defined(WIN16)
                SendMessage(hwndParent, WM_VSCROLL, (WPARAM)SB_ENDSCROLL, MAKELPARAM(GetWindowWord(hwnd, GWW_ID), hwnd));
#else
                /* See comments about WM_VSCROLL earlier in file */
                SendMessage( hwndParent
                           , WM_VSCROLL
                           , (WPARAM)SB_ENDSCROLL
                           , (LPARAM)hwnd
                           );
#endif //WIN16
                fButton = FALSE;
                if (ArrowType == enumArrowDown)
                        InvalidateRect(hwnd, &rcDown, TRUE);
                else
                        InvalidateRect(hwnd, &rcUp, TRUE);
        }
        fRealButton = FALSE;
        if (uTimer) {
                KillTimer(hwnd, uTimer);
                uTimer = 0;
                ReleaseCapture();
        }
}

/*--------------------------------------------------------------------------*/

static  void PASCAL NEAR Paint(
        HWND    hwnd)
{
        PAINTSTRUCT     ps;

        BeginPaint(hwnd, &ps);
        if (IsWindowVisible(hwnd)) {
                RECT    rcArrow;
                RECT    rcHalf;
                UINT    uMiddle;
                HBRUSH  hbrOld;
                int     iLoop;
                BOOL    fCurrentButtonDown;
                HPEN    hpenOld;

                GetClientRect(hwnd, &rcArrow);
                FrameRect(ps.hdc, &rcArrow, (HBRUSH)GetStockObject(BLACK_BRUSH));
                InflateRect(&rcArrow, -1, -1);
                // Create the barrier between the two buttons...;
                uMiddle = rcArrow.top + (rcArrow.bottom - rcArrow.top) / 2 + 1;
                hbrOld = (HBRUSH)SelectObject(ps.hdc, (HGDIOBJ)CreateSolidBrush(COLOR_WINDOWFRAME));
                PatBlt(ps.hdc, 0, rcArrow.bottom / 2 - 1, rcArrow.right, 2, PATCOPY);
                DeleteObject(SelectObject(ps.hdc, (HGDIOBJ)hbrOld));
                // Draw the shadows and the face of the button...;
                for (iLoop = enumArrowUp; iLoop <= enumArrowDown; iLoop++) {
                        POINT   ptArrow[3];
                        DWORD   dwColor;

                        fCurrentButtonDown = (fButton && (iLoop == ArrowType));
                        // get the rectangle for the button half we're dealing with;
                        rcHalf.top = (iLoop == enumArrowDown) ? uMiddle : rcArrow.top;
                        rcHalf.bottom = (iLoop == enumArrowDown) ? rcArrow.bottom : uMiddle - 2;
                        rcHalf.right = rcArrow.right;
                        rcHalf.left = rcArrow.left;
                        // draw the highlight lines;
                        if (fCurrentButtonDown)
                                dwColor = GetSysColor(COLOR_BTNSHADOW);
                        else
                                dwColor = RGB(255, 255, 255);
                        hpenOld = SelectObject(ps.hdc, (HGDIOBJ)CreatePen(PS_SOLID, 1, dwColor));
                        MMoveTo(ps.hdc, rcHalf.right - 1, rcHalf.top);
                        LineTo(ps.hdc, rcHalf.left, rcHalf.top);
                        LineTo(ps.hdc, rcHalf.left, rcHalf.bottom - 1 + fCurrentButtonDown);
                        DeleteObject(SelectObject(ps.hdc, (HGDIOBJ)hpenOld));
                        if (!fCurrentButtonDown) {
                                // draw the shadow lines;
                                hpenOld = SelectObject(ps.hdc, (HGDIOBJ)CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW)));
                                MMoveTo(ps.hdc, rcHalf.right - 1, rcHalf.top);
                                LineTo(ps.hdc, rcHalf.right - 1, rcHalf.bottom - 1);
                                LineTo(ps.hdc, rcHalf.left - 1, rcHalf.bottom - 1);
                                MMoveTo(ps.hdc, rcHalf.right - 2, rcHalf.top + 1);
                                LineTo(ps.hdc, rcHalf.right - 2, rcHalf.bottom - 2);
                                LineTo(ps.hdc, rcHalf.left, rcHalf.bottom - 2);
                                DeleteObject(SelectObject(ps.hdc, hpenOld));
                        }
                        // calculate the arrow triangle coordinates;
                        ptArrow[0].x = rcHalf.left + (rcHalf.right - rcHalf.left) / 2 + fCurrentButtonDown;
                        ptArrow[0].y = rcHalf.top + 2 + fCurrentButtonDown;
                        ptArrow[1].y = ptArrow[2].y = rcHalf.bottom - 4 + fCurrentButtonDown;
                        if (ptArrow[0].y > ptArrow[1].y)
                                ptArrow[1].y = ptArrow[2].y = ptArrow[0].y;
                        ptArrow[1].x = ptArrow[0].x - (ptArrow[1].y - ptArrow[0].y);
                        ptArrow[2].x = ptArrow[0].x + (ptArrow[1].y - ptArrow[0].y);
                        // flip over if we're drawing bottom button;
                        if (iLoop == enumArrowDown) {
                                ptArrow[2].y = ptArrow[0].y;
                                ptArrow[0].y = ptArrow[1].y;
                                ptArrow[1].y = ptArrow[2].y;
                        }
                        if (IsWindowEnabled(hwnd))
                                dwColor = GetSysColor(COLOR_BTNTEXT);
                        else
                                dwColor = GetSysColor(COLOR_GRAYTEXT);
                        // draw the triangle;
                        hbrOld = SelectObject(ps.hdc, (HGDIOBJ)CreateSolidBrush(dwColor));
                        hpenOld = SelectObject(ps.hdc, CreatePen(PS_SOLID, 1, dwColor));
                        Polygon(ps.hdc, ptArrow, 3);
                        DeleteObject(SelectObject(ps.hdc, (HGDIOBJ)hbrOld));
                        DeleteObject(SelectObject(ps.hdc, (HGDIOBJ)hpenOld));
                }
        }
        EndPaint(hwnd, &ps);
}

/*--------------------------------------------------------------------------*/

LRESULT PASCAL FAR _loadds ArrowControlProc(
        HWND    hwndArrow,
        UINT    wMsg,
        WPARAM  wParam,
        LPARAM  lParam)
{
        switch (wMsg) {
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_ENABLE:
        case WM_SYSCOLORCHANGE:
                InvalidateRect(hwndArrow, NULL, TRUE);
                UpdateWindow(hwndArrow);
                break;
        case WM_GETDLGCODE:
                return (LRESULT)DLGC_WANTARROWS; // | DLGC_UNDEFPUSHBUTTON;
        case WM_KEYDOWN:
                KeyDown(hwndArrow, wParam);
                break;
        case WM_KEYUP:
                KeyUp(hwndArrow, wParam);
                break;
        case WM_LBUTTONDOWN:
                LButtonDown(hwndArrow, HIWORD(lParam));   // y coord
                break;
        case WM_MOUSEMOVE:
                {    POINT pt;
                     LONG2POINT(lParam,pt);
                     MouseMove(hwndArrow, pt);
                }
                break;
        case WM_LBUTTONUP:
                LButtonUp(hwndArrow);
                break;
        case WM_TIMER:
                if (fButton)
#if defined(WIN16)
                        SendMessage(hwndParent, WM_VSCROLL, (WPARAM)uScroll, MAKELPARAM(GetWindowWord(hwndArrow, GWW_ID), hwndArrow));
#else
                        /* See comments about WM_VSCROLL earlier in file */
                        SendMessage( hwndParent
                                   , WM_VSCROLL
                                   , (WPARAM)uScroll
                                   , (LPARAM)hwndArrow
                                   );
#endif //WIN16
                break;
        case WM_PAINT:
                Paint(hwndArrow);
                break;
        default:
                return DefWindowProc(hwndArrow, wMsg, wParam, lParam);
        }
        return (LRESULT)0;
}

/*--------------------------------------------------------------------------*/

BOOL    PASCAL FAR RegisterArrowClass(
        HINSTANCE       hInstance)
{
        WNDCLASS        wcArrow;

        wcArrow.lpszClassName = szArrowClass;
        wcArrow.hInstance = hInstance;
        wcArrow.lpfnWndProc = (WNDPROC)ArrowControlProc;
        wcArrow.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcArrow.hIcon = NULL;
        wcArrow.lpszMenuName = NULL;
        wcArrow.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wcArrow.style = CS_HREDRAW | CS_VREDRAW;
        wcArrow.cbClsExtra = 0;
        wcArrow.cbWndExtra = 0;
        return RegisterClass((LPWNDCLASS)&wcArrow);
}

/*--------------------------------------------------------------------------*/

void    PASCAL FAR UnregisterArrowClass(
        HINSTANCE       hInstance)
{
        UnregisterClass(szArrowClass, hInstance);
}
