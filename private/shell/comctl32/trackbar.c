//----------------------------------------------------------
//
// BUGBUG: make sure this stuff really works with the DWORD
//         ranges
//
//----------------------------------------------------------

#include "ctlspriv.h"
#include "limits.h"
#include "image.h"          // for CreateColorBitmap

#if defined(MAINWIN)
#include <mainwin.h>
#endif

//#define TB_DEBUG
//#define FEATURE_DEBUG     // Ctrl+Shift force-enables rare features for debugging

typedef struct {

    // standard header information for each control
    CONTROLINFO ci;

    HDC     hdc;            // current DC
    HBITMAP hbmBuffer;      // double buffer

    LONG    lLogMin;        // Logical minimum
    LONG    lLogMax;        // Logical maximum
    LONG    lLogPos;        // Logical position

    LONG    lSelStart;      // Logical selection start
    LONG    lSelEnd;        // Logical selection end

    int     iThumbWidth;    // Width of the thumb
    int     iThumbHeight;   // Height of the thumb

    int     iSizePhys;      // Size of where thumb lives
    RECT    rc;             // track bar rect.

    RECT    rcThumb;          // Rectangle we current thumb
    DWORD   dwDragPos;      // Logical position of mouse while dragging.
    int     dwDragOffset;   // how many pixels off the center did they click

    int     nTics;          // number of ticks.
    PDWORD  pTics;          // the tick marks.

    int     ticFreq;        // the frequency of ticks

    LONG     lPageSize;      // how much to thumb up and down.
    LONG     lLineSize;      // how muhc to scroll up and down on line up/down

    HWND     hwndToolTips;

    // these should probably be word or bytes
    UINT     wDirtyFlags;
    UINT     uTipSide;   // which side should the tip be on?
    UINT     Flags;          // Flags for our window
    UINT     Cmd;            // The command we're repeating.


#if defined(FE_IME) || !defined(WINNT)
    HIMC    hPrevImc;       // previous input context handle
#endif


    HWND        hwndBuddyLeft;
    HWND        hwndBuddyRight;

} TRACKBAR, *PTRACKBAR;

// Trackbar flags

#define TBF_NOTHUMB     0x0001  // No thumb because not wide enough.
#define TBF_SELECTION   0x0002  // a selection has been established (draw the range)

#define MIN_THUMB_HEIGHT (2 * g_cxEdge)

/*
        useful constants.
*/

#define REPEATTIME      500     // mouse auto repeat 1/2 of a second
#define TIMER_ID        1

/*
        Function Prototypes
*/

void   NEAR PASCAL DoTrack(PTRACKBAR, int, DWORD);
WORD   NEAR PASCAL WTrackType(PTRACKBAR, LONG);
void   NEAR PASCAL TBTrackInit(PTRACKBAR, LPARAM);
void   NEAR PASCAL TBTrackEnd(PTRACKBAR);
void   NEAR PASCAL TBTrack(PTRACKBAR, LPARAM);
void   NEAR PASCAL DrawThumb(PTRACKBAR, LPRECT, BOOL);

HBRUSH NEAR PASCAL SelectColorObjects(PTRACKBAR, BOOL);
void   NEAR PASCAL SetTBCaretPos(PTRACKBAR);

#define TICKHEIGHT 3
#define BORDERSIZE 2

#define ISVERT(tb) (tb->ci.style & TBS_VERT)

#define TBC_TICS        0x1
#define TBC_THUMB       0x2
#define TBC_ALL         0xF


// this is called internally when the trackbar has
// changed and we need to update the double buffer bitmap
// we only set a flag.  we do the actual draw
// during WM_PAINT.  This prevents wasted efforts drawing.
#define TBChanged(ptb, wFlags) ((ptb)->wDirtyFlags |= (wFlags))

//
// Function Prototypes
//
LPARAM FAR CALLBACK TrackBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL FlushChanges(PTRACKBAR tb);

//--------------------------------------------------------------------------;
//
//  LONG MulDiv32(a,b,c)    = (a * b + c/2) / c
//
//--------------------------------------------------------------------------;

#ifdef WIN32

#define MulDiv32 MulDiv     // use KERNEL32 version (it rounds)

#else // WIN32

#define ASM66 _asm _emit 0x66 _asm
#define DB    _asm _emit

#define EAX_TO_DXAX \
    DB      0x66    \
    DB      0x0F    \
    DB      0xA4    \
    DB      0xC2    \
    DB      0x10

#pragma warning(disable:4035 4704)

static LONG MulDiv32(LONG a,LONG b,LONG c)
{
    ASM66   mov     ax,word ptr c   //  mov  eax, c
    ASM66   sar     ax,1            //  sar  eax,1
    ASM66   cwd                     //  cdq
    ASM66   mov     bx,ax           //  mov  ebx,eax
    ASM66   mov     cx,dx           //  mov  ecx,edx
    ASM66   mov     ax,word ptr a   //  mov  eax, a
    ASM66   imul    word ptr b      //  imul b
    ASM66   add     ax,bx           //  add  eax,ebx
    ASM66   adc     dx,cx           //  adc  edx,ecx
    ASM66   idiv    word ptr c      //  idiv c
    EAX_TO_DXAX

} // MulDiv32()

#pragma warning(default:4035 4704)

#endif // WIN32

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//
//  convert a logical scroll-bar position to a physical pixel position
//
int NEAR PASCAL TBLogToPhys(PTRACKBAR tb, DWORD dwPos)
{
    int x;
    x = tb->rc.left;
    if (tb->lLogMax == tb->lLogMin)
        return x;

    return (int)MulDiv32(dwPos - tb->lLogMin, tb->iSizePhys - 1,
                          tb->lLogMax - tb->lLogMin) + x;
}

LONG NEAR PASCAL TBPhysToLog(PTRACKBAR ptb, int iPos)
{
    int min, max, x;
    min = ptb->rc.left;
    max = ptb->rc.right;
    x = ptb->rc.left;

    if (ptb->iSizePhys <= 1)
        return ptb->lLogMin;

    if (iPos <= min)
        return ptb->lLogMin;

    if (iPos >= max)
        return ptb->lLogMax;

    return MulDiv32(iPos - x, ptb->lLogMax - ptb->lLogMin,
                    ptb->iSizePhys - 1) + ptb->lLogMin;
}



#pragma code_seg(CODESEG_INIT)
/*
 * Initialize the trackbar code
 */

BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance)
{
    WNDCLASS wc;

    // See if we must register a window class
    if (!GetClassInfo(hInstance, s_szSTrackBarClass, &wc)) {
#ifndef WIN32
        extern LRESULT CALLBACK _TrackBarWndProc(HWND, UINT, WPARAM, LPARAM);
        wc.lpfnWndProc = _TrackBarWndProc;
#else
        wc.lpfnWndProc = TrackBarWndProc;
#endif

        wc.lpszClassName = s_szSTrackBarClass;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = NULL;
        wc.lpszMenuName = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.hInstance = hInstance;
        wc.style = CS_GLOBALCLASS;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(PTRACKBAR);

        return RegisterClass(&wc);
    }
    return TRUE;
}
#pragma code_seg()



/* 
 * To add vertical capabilities, I'm using a virtual coordinate
 * system.  the ptb->rcThumb and ptb->rc are in the virtual space (which
 * is just a horizontal trackbar).  Draw routines use PatRect and
 * TBBitBlt which switch to the real coordinate system as needed.
 *
 * The one gotcha is that the Thumb Bitmap has the pressed bitmap
 * to the real right, and the masks to the real right again for both
 * the vertical and horizontal Thumbs.  So those cases are hardcoded.
 * Do a search for ISVERT to find these dependancies.
 *                              -Chee
 */

/*
  FlipRect Function is moved to cutils.c as  other controls  were also using it.
  -Arul

*/

void TBFlipPoint(PTRACKBAR ptb, LPPOINT lppt)
{
    if (ISVERT(ptb)) {
        FlipPoint(lppt);
    }
}


/* added trackbar variable to do auto verticalization */
void NEAR PASCAL PatRect(HDC hdc,int x,int y,int dx,int dy, PTRACKBAR ptb)
{
    RECT    rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    if (ISVERT(ptb))
        FlipRect(&rc);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#define TBInvalidateRect(hwnd, prc, bErase, ptb) VertInvalidateRect(hwnd, prc, bErase, ISVERT(ptb))
void FAR PASCAL VertInvalidateRect(HWND hwnd, LPRECT qrc, BOOL b, BOOL fVert)
{
    RECT rc;
    rc = *qrc;
    if (fVert) FlipRect(&rc);
    InvalidateRect(hwnd, &rc, b);
}

#define TBDrawEdge(hdc, prc, uType, grfFlags, ptb) VertDrawEdge(hdc, prc, uType, grfFlags, ISVERT(ptb))
void FAR PASCAL VertDrawEdge(HDC hdc, LPRECT qrc, UINT edgeType, UINT grfFlags,
                               BOOL fVert)
{
    RECT temprc;
    UINT uFlags = grfFlags;

    temprc = *qrc;
    if (fVert) {
        FlipRect(&temprc);

        if (!(uFlags & BF_DIAGONAL)) {
            if (grfFlags & BF_TOP) uFlags |= BF_LEFT;
            else uFlags &= ~BF_LEFT;

            if (grfFlags & BF_LEFT) uFlags |= BF_TOP;
            else uFlags &= ~BF_TOP;

            if (grfFlags & BF_BOTTOM) uFlags |= BF_RIGHT;
            else uFlags &= ~BF_RIGHT;

            if (grfFlags & BF_RIGHT) uFlags |= BF_BOTTOM;
            else uFlags &= ~BF_BOTTOM;
        } else {
            if ((grfFlags & (BF_BOTTOM | BF_RIGHT)) == (BF_BOTTOM | BF_RIGHT)) {
                uFlags = BF_TOP | BF_LEFT;

                if (edgeType == EDGE_RAISED) {
                    edgeType = EDGE_SUNKEN;
                } else {
                    edgeType = EDGE_RAISED;
                }


                uFlags |= grfFlags & (~BF_RECT);
                uFlags ^= BF_SOFT;
            }
        }
    }
    DrawEdge(hdc, &temprc, edgeType, uFlags);
}

void NEAR PASCAL TBBitBlt(HDC hdc1, int x1, int y1, int w, int h,
                          HDC hdc2, int x2, int y2, DWORD rop, PTRACKBAR ptb)
{
    if (ISVERT(ptb))
        BitBlt(hdc1, y1, x1, h, w, hdc2, x2, y2, rop);
    else
        BitBlt(hdc1, x1, y1, w, h, hdc2, x2, y2, rop);
}

#define TBPatBlt(hdc1, x1, y1, w, h, rop, ptb) VertPatBlt(hdc1, x1, y1, w, h, rop, ISVERT(ptb))
void FAR PASCAL VertPatBlt(HDC hdc1, int x1, int y1, int w, int h,
                          DWORD rop, BOOL fVert)
{
    if (fVert)
        PatBlt(hdc1, y1, x1, h, w, rop);
    else
        PatBlt(hdc1, x1, y1, w, h, rop);
}


void NEAR PASCAL DrawTic(PTRACKBAR ptb, int x, int y, int dir)
{
    if (dir == -1) y -= TICKHEIGHT;
    SetBkColor(ptb->hdc, g_clrBtnText);
    PatRect(ptb->hdc,x,y,1,TICKHEIGHT, ptb);
}

// dir = direction multiplier (drawing up or down)
// yTic = where (vertically) to draw the line of tics
void NEAR PASCAL DrawTicsOneLine(PTRACKBAR ptb, int dir, int yTic)
{
    PDWORD pTics;
    int    iPos;
    int    i;

    DrawTic(ptb, ptb->rc.left, yTic, dir);             // first
    DrawTic(ptb, ptb->rc.left, yTic+ (dir * 1), dir);
    DrawTic(ptb, ptb->rc.right-1, yTic, dir);            // last
    DrawTic(ptb, ptb->rc.right-1, yTic+ (dir * 1), dir);

    // those inbetween
    pTics = ptb->pTics;
    if (ptb->ticFreq && pTics) {
        for (i = 0; i < ptb->nTics; ++i) {
            if (((i+1) % ptb->ticFreq) == 0) {
                iPos = TBLogToPhys(ptb,pTics[i]);
                DrawTic(ptb, iPos, yTic, dir);
            }
        }
    }

    // draw the selection range (triangles)

    if ((ptb->Flags & TBF_SELECTION) &&
        (ptb->lSelStart <= ptb->lSelEnd) && (ptb->lSelEnd >= ptb->lLogMin)) {

        SetBkColor(ptb->hdc, g_clrBtnText);

        iPos = TBLogToPhys(ptb,ptb->lSelStart);

        for (i = 0; i < TICKHEIGHT; i++)
            PatRect(ptb->hdc,iPos-i,yTic+(dir==1 ? i : -TICKHEIGHT),
                    1,TICKHEIGHT-i, ptb);

        iPos = TBLogToPhys(ptb,ptb->lSelEnd);

        for (i = 0; i < TICKHEIGHT; i++)
            PatRect(ptb->hdc,iPos+i,yTic+(dir==1 ? i : -TICKHEIGHT),
                    1,TICKHEIGHT-i, ptb);
    }

}

/* DrawTics() */
/* There is always a tick at the beginning and end of the bar, but you can */
/* add some more of your own with a TBM_SETTIC message.  This draws them.  */
/* They are kept in an array whose handle is a window word.  The first     */
/* element is the number of extra ticks, and then the positions.           */

void NEAR PASCAL DrawTics(PTRACKBAR ptb)
{
    // do they even want this?
    if (ptb->ci.style & TBS_NOTICKS) return;

    if ((ptb->ci.style & TBS_BOTH) || !(ptb->ci.style & TBS_TOP)) {
        DrawTicsOneLine(ptb, 1, ptb->rc.bottom + 1);
    }

    if ((ptb->ci.style & (TBS_BOTH | TBS_TOP))) {
        DrawTicsOneLine(ptb, -1, ptb->rc.top - 1);
    }
}

void NEAR PASCAL GetChannelRect(PTRACKBAR ptb, LPRECT lprc)
{
        int iwidth, iheight;

        if (!lprc)
            return;

        lprc->left = ptb->rc.left - ptb->iThumbWidth / 2;
        iwidth = ptb->iSizePhys + ptb->iThumbWidth - 1;
        lprc->right = lprc->left + iwidth;

        if (ptb->ci.style & TBS_ENABLESELRANGE) {
                iheight =  ptb->iThumbHeight / 4 * 3; // this is Scrollheight
        } else {
                iheight = 4;
        }

        lprc->top = (ptb->rc.top + ptb->rc.bottom - iheight) /2;
        if (!(ptb->ci.style & TBS_BOTH))
            if (ptb->ci.style & TBS_TOP) lprc->top++;
            else lprc->top--;

        lprc->bottom = lprc->top + iheight;

}

/* This draws the track bar itself */

void NEAR PASCAL DrawChannel(PTRACKBAR ptb, LPRECT lprc)
{

        TBDrawEdge(ptb->hdc, lprc, EDGE_SUNKEN, BF_RECT,ptb);

        SetBkColor(ptb->hdc, g_clrBtnHighlight);
        // Fill the center
        PatRect(ptb->hdc, lprc->left+2, lprc->top+2, (lprc->right-lprc->left)-4,
                (lprc->bottom-lprc->top)-4, ptb);


        // now highlight the selection range
        if ((ptb->Flags & TBF_SELECTION) &&
            (ptb->lSelStart <= ptb->lSelEnd) && (ptb->lSelEnd > ptb->lLogMin)) {
                int iStart, iEnd;

                iStart = TBLogToPhys(ptb,ptb->lSelStart);
                iEnd   = TBLogToPhys(ptb,ptb->lSelEnd);

                if (iStart + 2 <= iEnd) {
                        SetBkColor(ptb->hdc, g_clrHighlight);
                        PatRect(ptb->hdc, iStart+1, lprc->top+3,
                                iEnd-iStart-1, (lprc->bottom-lprc->top)-6, ptb);
                }
        }
}

void NEAR PASCAL DrawThumb(PTRACKBAR ptb, LPRECT lprc, BOOL fSelected)
{

    // iDpt direction from middle to point of thumb
    // a negative value inverts things.
    // this allows one code path..
    int iDpt = 0;
    int i = 0;  // size of point triangle
    int iYpt = 0;       // vertical location of tip;
    int iXmiddle = 0;
    int icount;  // just a loop counter
    UINT uEdgeFlags;
    RECT rcThumb = *lprc;

    if (ptb->Flags & TBF_NOTHUMB ||
        ptb->ci.style & TBS_NOTHUMB)            // If no thumb, just leave.
        return;

    ASSERT(ptb->iThumbHeight >= MIN_THUMB_HEIGHT);
    ASSERT(ptb->iThumbWidth > 1);


    // draw the rectangle part
    if (!(ptb->ci.style & TBS_BOTH))  {
        int iMiddle;
        // do -3  because wThumb is odd (triangles ya know)
        // and because draw rects draw inside the rects passed.
        // actually should be (width-1)/2-1, but this is the same...

        i = (ptb->iThumbWidth - 3) / 2;
        iMiddle = ptb->iThumbHeight / 2 + rcThumb.top;

        //draw the rectangle part
        if (ptb->ci.style & TBS_TOP) {
            iMiddle++; //correction because drawing routines
            iDpt = -1;
            rcThumb.top += (i+1);
            uEdgeFlags = BF_SOFT | BF_LEFT | BF_RIGHT | BF_BOTTOM;
        } else {
            iDpt = 1;
            rcThumb.bottom -= (i+1);
            // draw on the inside, not on the bottom and rt edge
            uEdgeFlags = BF_SOFT | BF_LEFT | BF_RIGHT | BF_TOP;
        }

        iYpt = iMiddle + (iDpt * (ptb->iThumbHeight / 2));
        iXmiddle = rcThumb.left + i;
    }  else {
        uEdgeFlags = BF_SOFT | BF_RECT;
    }

    // fill in the center
    if (fSelected || !IsWindowEnabled(ptb->ci.hwnd)) {
        HBRUSH hbrTemp;
        // draw the dithered insides;
        hbrTemp = SelectObject(ptb->hdc, g_hbrMonoDither);
        if (hbrTemp) {
            SetTextColor(ptb->hdc, g_clrBtnHighlight);
            SetBkColor(ptb->hdc, g_clrBtnFace);
            TBPatBlt(ptb->hdc, rcThumb.left +2 , rcThumb.top,
                     rcThumb.right-rcThumb.left -4, rcThumb.bottom-rcThumb.top,
                     PATCOPY,ptb);

            if (!(ptb->ci.style & TBS_BOTH)) {

                for (icount = 1;  icount <= i;  icount++) {
                    TBPatBlt(ptb->hdc, iXmiddle-icount+1,
                         iYpt - (iDpt*icount),
                         icount*2, 1, PATCOPY, ptb);
                }
            }
            SelectObject(ptb->hdc, hbrTemp);
        }

    } else {


        SetBkColor(ptb->hdc, g_clrBtnFace);
        PatRect(ptb->hdc, rcThumb.left+2, rcThumb.top,
                rcThumb.right-rcThumb.left-4, rcThumb.bottom-rcThumb.top, ptb);

        if (!(ptb->ci.style & TBS_BOTH)) {
            for (icount = 1; icount <= i; icount++) {
                PatRect(ptb->hdc, iXmiddle-icount+1,
                        iYpt - (iDpt*icount),
                        icount*2, 1, ptb);
            }
        }

    }

    TBDrawEdge(ptb->hdc, &rcThumb, EDGE_RAISED, uEdgeFlags, ptb);


    //now draw the point
    if (!(ptb->ci.style & TBS_BOTH)) {
        UINT uEdgeFlags2;

        // uEdgeFlags is now used to switch between top and bottom.
        // we'll or it in with the diagonal and left/right flags below
        if (ptb->ci.style & TBS_TOP) {
            rcThumb.bottom = rcThumb.top + 1;
            rcThumb.top = rcThumb.bottom - (i + 2);
            uEdgeFlags = BF_TOP | BF_RIGHT | BF_DIAGONAL | BF_SOFT;
            uEdgeFlags2 = BF_BOTTOM | BF_RIGHT | BF_DIAGONAL;
        } else {
            rcThumb.top = rcThumb.bottom - 1;
            rcThumb.bottom = rcThumb.top + (i + 2);

            uEdgeFlags = BF_TOP | BF_LEFT | BF_DIAGONAL | BF_SOFT;
            uEdgeFlags2 = BF_BOTTOM | BF_LEFT | BF_DIAGONAL;
        }

        rcThumb.right = rcThumb.left + (i + 2);
        // do the left side first
        TBDrawEdge(ptb->hdc, &rcThumb, EDGE_RAISED, uEdgeFlags , ptb);
        // then do th right side
        OffsetRect(&rcThumb, i + 1, 0);
        TBDrawEdge(ptb->hdc, &rcThumb, EDGE_RAISED, uEdgeFlags2 , ptb);
    }
}
void NEAR PASCAL TBInvalidateAll(PTRACKBAR ptb)
{
    if (ptb) {
        TBChanged(ptb, TBC_ALL);
        InvalidateRect(ptb->ci.hwnd, NULL, FALSE);
    }
}

void NEAR PASCAL MoveThumb(PTRACKBAR ptb, LONG lPos)
{
    long    lOld = ptb->lLogPos;

    TBInvalidateRect(ptb->ci.hwnd, &ptb->rcThumb, FALSE,ptb);

    ptb->lLogPos  = BOUND(lPos,ptb->lLogMin,ptb->lLogMax);
    ptb->rcThumb.left   = TBLogToPhys(ptb, ptb->lLogPos) - ptb->iThumbWidth / 2;
    ptb->rcThumb.right  = ptb->rcThumb.left + ptb->iThumbWidth;

    TBInvalidateRect(ptb->ci.hwnd, &ptb->rcThumb, FALSE,ptb);
    TBChanged(ptb, TBC_THUMB);
    UpdateWindow(ptb->ci.hwnd);

    if (lOld != ptb->lLogPos)
        MyNotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ptb->ci.hwnd, OBJID_CLIENT, 0);
}


void NEAR PASCAL DrawFocus(PTRACKBAR ptb, HBRUSH hbrBackground)
{
    RECT rc;
    if (ptb->ci.hwnd == GetFocus()
#ifdef KEYBOARDCUES
        && !(CCGetUIState(&(ptb->ci)) & UISF_HIDEFOCUS)
#endif
        )
    {
        SetBkColor(ptb->hdc, g_clrBtnHighlight);
        GetClientRect(ptb->ci.hwnd, &rc);

        // Successive calls to DrawFocusRect will invert it thereby erasing it.
        // To avoid this, whenever we process WM_PAINT, we erase the focus rect ourselves
        // before we draw it below.
        if (hbrBackground)
            FrameRect(ptb->hdc, &rc, hbrBackground);

        DrawFocusRect(ptb->hdc, &rc);
    }
}

void NEAR PASCAL DoAutoTics(PTRACKBAR ptb)
{
    LONG NEAR *pl;
    LONG l;

    if (!(ptb->ci.style & TBS_AUTOTICKS))
        return;

    if (ptb->pTics)
        LocalFree((HLOCAL)ptb->pTics);

    ptb->nTics = (int)(ptb->lLogMax - ptb->lLogMin - 1);

    if (ptb->nTics > 0)
        ptb->pTics = (DWORD NEAR *)LocalAlloc(LPTR, sizeof(DWORD) * ptb->nTics);
    else
        ptb->pTics = NULL;

    if (!ptb->pTics) {
        ptb->nTics = 0;
        return;
    }

    for (pl = (LONG NEAR *)ptb->pTics, l = ptb->lLogMin + 1; l < ptb->lLogMax; l++)
        *pl++ = l;
}


void NEAR PASCAL ValidateThumbHeight(PTRACKBAR ptb)
{
    if (ptb->iThumbHeight < MIN_THUMB_HEIGHT)
        ptb->iThumbHeight = MIN_THUMB_HEIGHT;

    ptb->iThumbWidth = ptb->iThumbHeight / 2;
    ptb->iThumbWidth |= 0x01;  // make sure it's odd at at least 3

    if (ptb->ci.style & TBS_ENABLESELRANGE) {
        if (ptb->ci.style & TBS_FIXEDLENGTH) {
            // half of 9/10
            ptb->iThumbWidth = (ptb->iThumbHeight * 9) / 20;
            ptb->iThumbWidth |= 0x01;
        } else {
            ptb->iThumbHeight += (ptb->iThumbWidth * 2) / 9;
        }
    }
}

void TBPositionBuddies(PTRACKBAR ptb)
{
    POINT pt;
    HWND hwndParent;
    RECT rcBuddy;
    RECT rcClient;
    RECT rcChannel;

    int yMid;

    GetChannelRect(ptb, &rcChannel);
    yMid = (rcChannel.top + rcChannel.bottom) / 2;

    GetClientRect(ptb->ci.hwnd, &rcClient);
    if (ISVERT(ptb))
        FlipRect(&rcClient);


    if (ptb->hwndBuddyLeft) {
        GetClientRect(ptb->hwndBuddyLeft, &rcBuddy);
        if (ISVERT(ptb))
            FlipRect(&rcBuddy);

        pt.y = yMid - ((RECTHEIGHT(rcBuddy))/2);
        pt.x = rcClient.left - RECTWIDTH(rcBuddy) - g_cxEdge;

        // x and y are now in trackbar's coordinates.
        // convert them to the parent of the buddy's coordinates
        hwndParent = GetParent(ptb->hwndBuddyLeft);
        TBFlipPoint(ptb, &pt);
        MapWindowPoints(ptb->ci.hwnd, hwndParent, &pt, 1);
        SetWindowPos(ptb->hwndBuddyLeft, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE |SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (ptb->hwndBuddyRight) {
        GetClientRect(ptb->hwndBuddyRight, &rcBuddy);
        if (ISVERT(ptb))
            FlipRect(&rcBuddy);

        pt.y = yMid - ((RECTHEIGHT(rcBuddy))/2);
        pt.x = rcClient.right + g_cxEdge;

        // x and y are now in trackbar's coordinates.
        // convert them to the parent of the buddy's coordinates
        hwndParent = GetParent(ptb->hwndBuddyRight);
        TBFlipPoint(ptb, &pt);
        MapWindowPoints(ptb->ci.hwnd, hwndParent, &pt, 1);
        SetWindowPos(ptb->hwndBuddyRight, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE |SWP_NOZORDER | SWP_NOACTIVATE);
    }

}

void NEAR PASCAL TBNukeBuffer(PTRACKBAR ptb)
{
    if (ptb->hbmBuffer) {
        DeleteObject(ptb->hbmBuffer);
        ptb->hbmBuffer = NULL;
        TBChanged(ptb, TBC_ALL);            // Must do a full repaint
    }
}

void NEAR PASCAL TBResize(PTRACKBAR ptb)
{
    GetClientRect(ptb->ci.hwnd, &ptb->rc);

    if (ISVERT(ptb))
        FlipRect(&ptb->rc);


    if (!(ptb->ci.style & TBS_FIXEDLENGTH)) {
        ptb->iThumbHeight = (g_cyHScroll * 4) / 3;

        ValidateThumbHeight(ptb);
        if ((ptb->iThumbHeight > MIN_THUMB_HEIGHT) && (ptb->rc.bottom < (int)ptb->iThumbHeight)) {
            ptb->iThumbHeight = ptb->rc.bottom - 3*g_cyEdge; // top, bottom, and tic
            if (ptb->ci.style & TBS_ENABLESELRANGE)
                ptb->iThumbHeight = (ptb->iThumbHeight * 3 / 4);
            ValidateThumbHeight(ptb);
        }
    } else {
        ValidateThumbHeight(ptb);
    }


    if (ptb->ci.style & (TBS_BOTH | TBS_TOP) && !(ptb->ci.style & TBS_NOTICKS))
        ptb->rc.top += TICKHEIGHT + BORDERSIZE + 3;
    ptb->rc.top   += BORDERSIZE;
    ptb->rc.bottom  = ptb->rc.top + ptb->iThumbHeight;
    ptb->rc.left   += (ptb->iThumbWidth + BORDERSIZE);
    ptb->rc.right  -= (ptb->iThumbWidth + BORDERSIZE);

    ptb->rcThumb.top = ptb->rc.top;
    ptb->rcThumb.bottom = ptb->rc.bottom;

    // Figure out how much room we have to move the thumb in
    ptb->iSizePhys = ptb->rc.right - ptb->rc.left;

    // Elevator isn't there if there's no room.
    if (ptb->iSizePhys == 0) {
        // Lost our thumb.
        ptb->Flags |= TBF_NOTHUMB;
        ptb->iSizePhys = 1;
    } else {
        // Ah. We have a thumb.
        ptb->Flags &= ~TBF_NOTHUMB;
    }

    TBNukeBuffer(ptb);

    MoveThumb(ptb, ptb->lLogPos);
    TBInvalidateAll(ptb);

    TBPositionBuddies(ptb);
}

LRESULT NEAR PASCAL TrackOnCreate(HWND hwnd, LPCREATESTRUCT lpCreate)
{
    PTRACKBAR       ptb;

#ifdef MAINWIN
    DWORD exStyle = WS_EX_MW_UNMANAGED_WINDOW;
#else
    DWORD exStyle = 0;
#endif

    InitDitherBrush();
    InitGlobalColors();

    // Get us our window structure.
    ptb = (PTRACKBAR)LocalAlloc(LPTR, sizeof(TRACKBAR));
    if (!ptb)
        return -1;

    SetWindowPtr(hwnd, 0, ptb);
    CIInitialize(&ptb->ci, hwnd, lpCreate);

    ptb->Cmd = (UINT)-1;
    ptb->lLogMax = 100;
    ptb->ticFreq = 1;
    // ptb->hbmBuffer = 0;
    ptb->lPageSize = -1;
    ptb->lLineSize = 1;
    // initial size;
    ptb->iThumbHeight = (g_cyHScroll * 4) / 3;
#if defined(FE_IME) || !defined(WINNT)
    if (g_fDBCSInputEnabled)
        ptb->hPrevImc = ImmAssociateContext(hwnd, 0L);
#endif

#ifdef FEATURE_DEBUG
    if (GetAsyncKeyState(VK_SHIFT) < 0 &&
        GetAsyncKeyState(VK_CONTROL) < 0)
        ptb->ci.style |= TBS_TOOLTIPS;
#endif

    if (ISVERT(ptb)) {
        if (ptb->ci.style & TBS_TOP) {
            ptb->uTipSide = TBTS_RIGHT;
        } else {
            ptb->uTipSide = TBTS_LEFT;
        }
    } else {
        if (ptb->ci.style & TBS_TOP) {
            ptb->uTipSide = TBTS_BOTTOM;
        } else {
            ptb->uTipSide = TBTS_TOP;
        }
    }

    if (ptb->ci.style & TBS_TOOLTIPS) {
        ptb->hwndToolTips = CreateWindowEx(exStyle, 
                                              c_szSToolTipsClass, TEXT(""),
                                              WS_POPUP,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              ptb->ci.hwnd, NULL, HINST_THISDLL,
                                              NULL);
        if (ptb->hwndToolTips) {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in FlushToolTipsMgr;
            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_CENTERTIP;
            ti.hwnd = ptb->ci.hwnd;
            ti.uId = (UINT_PTR)ptb->ci.hwnd;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0; // update this on size
            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        } else
            ptb->ci.style &= ~(TBS_TOOLTIPS);
    }


    TBResize(ptb);

#ifdef FEATURE_DEBUG
    if (GetAsyncKeyState(VK_SHIFT) < 0 &&
        GetAsyncKeyState(VK_CONTROL) < 0 )
    {
        HWND hwnd =
            CreateWindowEx(WS_EX_STATICEDGE, TEXT("static"), TEXT("left"), WS_CHILD | WS_VISIBLE, 0, 0, 30, 20, GetParent(ptb->ci.hwnd), NULL, HINST_THISDLL, NULL);
        HWND hwnd2 =
            CreateWindowEx(WS_EX_STATICEDGE, TEXT("static"), TEXT("right"), WS_CHILD |WS_VISIBLE, 0, 0, 50, 20, GetParent(ptb->ci.hwnd), NULL, HINST_THISDLL, NULL);

        SendMessage(ptb->ci.hwnd, TBM_SETBUDDY, TRUE, (LPARAM)hwnd);
        SendMessage(ptb->ci.hwnd, TBM_SETBUDDY, FALSE, (LPARAM)hwnd2);
    }
#endif


    return 0;
}

void NEAR PASCAL TrackOnNotify(PTRACKBAR ptb, LPNMHDR lpnm)
{
    if (lpnm->hwndFrom == ptb->hwndToolTips) {
        switch (lpnm->code) {
        case TTN_NEEDTEXT:
#define lpttt ((LPTOOLTIPTEXT)lpnm)
            wsprintf(lpttt->szText, TEXT("%d"), ptb->lLogPos);

        default:
            SendNotifyEx(ptb->ci.hwndParent, (HWND)-1,
                         lpnm->code, lpnm, ptb->ci.bUnicode);
            break;
        }
    }
}

HWND TBSetBuddy(PTRACKBAR ptb, BOOL fLeft, HWND hwndBuddy)
{
    HWND hwndOldBuddy;

    if (fLeft) {
        hwndOldBuddy = ptb->hwndBuddyLeft;
        ptb->hwndBuddyLeft = hwndBuddy;
    } else {
        hwndOldBuddy = ptb->hwndBuddyRight;
        ptb->hwndBuddyRight = hwndBuddy;
    }

    TBResize(ptb);

    return hwndOldBuddy;
}

LPARAM FAR CALLBACK TrackBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        PTRACKBAR       ptb;
        PAINTSTRUCT     ps;
        HLOCAL          h;

        ptb = GetWindowPtr(hwnd, 0);
        if (!ptb) {
            if (uMsg == WM_CREATE)
                return TrackOnCreate(hwnd, (LPCREATESTRUCT)lParam);

            goto DoDefault;
        }

        switch (uMsg) {

        // If color depth changes, the old buffer is no longer any good
        case WM_DISPLAYCHANGE:
            TBNukeBuffer(ptb);
            break;

        case WM_WININICHANGE:

            InitGlobalMetrics(wParam);
            // fall through to WM_SIZE

        case WM_SIZE:
            TBResize(ptb);
            break;

        case WM_SYSCOLORCHANGE:
            InitGlobalColors();
            TBInvalidateAll(ptb);
            break;

        case WM_NOTIFYFORMAT:
            return CIHandleNotifyFormat(&ptb->ci,lParam);

        case WM_NOTIFY:
            TrackOnNotify(ptb, (LPNMHDR)lParam);
            break;

        case WM_DESTROY:
            TerminateDitherBrush();
            if (ptb) {
#if defined(FE_IME) || !defined(WINNT)
                if (g_fDBCSInputEnabled)
                    ImmAssociateContext(hwnd, ptb->hPrevImc);
#endif
                if ((ptb->ci.style & TBS_TOOLTIPS) && IsWindow(ptb->hwndToolTips)) {
                    DestroyWindow (ptb->hwndToolTips);
                }

                TBNukeBuffer(ptb);

                if (ptb->pTics)
                    LocalFree((HLOCAL)ptb->pTics);

                LocalFree((HLOCAL)ptb);
                SetWindowPtr(hwnd, 0, 0);
            }
            break;

        case WM_KILLFOCUS:
            // Reset wheel scroll amount
            gcWheelDelta = 0;
            // fall-through

        case WM_SETFOCUS:
            ASSERT(gcWheelDelta == 0);
            if (ptb)
                TBInvalidateAll(ptb);
            break;

        case WM_ENABLE:
            if (wParam) {
                ptb->ci.style &= ~WS_DISABLED;
            } else {
                ptb->ci.style |= WS_DISABLED;
            }
            TBChanged(ptb, TBC_THUMB);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_PRINTCLIENT:
        case WM_PAINT: {
            RECT rc;
            HBITMAP hbmOld;
            HDC hdc;

            hdc = wParam ?  (HDC)wParam : BeginPaint(hwnd, &ps);

            //DebugMsg(DM_TRACE, "NumTics = %d", SendMessage(ptb->ci.hwnd, TBM_GETNUMTICS, 0, 0));

            //ptb->hdc = GetDC(NULL);
            ptb->hdc = CreateCompatibleDC(hdc);
            if (!ptb->hbmBuffer) {
                GetClientRect(hwnd, &rc);
                ptb->hbmBuffer = CreateColorBitmap(rc.right, rc.bottom);
            }

            hbmOld = SelectObject(ptb->hdc, ptb->hbmBuffer);
            FlushChanges(ptb);

            //only copy the area that's changable.. ie the clip box
            switch(GetClipBox(hdc, &rc)) {
                case NULLREGION:
                case ERROR:
                    GetClientRect(ptb->ci.hwnd, &rc);
            }
            BitBlt(hdc, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top,
                     ptb->hdc, rc.left, rc.top, SRCCOPY);

#ifdef TB_DEBUG
            {
                HDC hdcScreen;
                RECT rcClient;
                hdcScreen = GetDC(NULL);
                GetClientRect(ptb->ci.hwnd, &rcClient);
                BitBlt(hdcScreen, 0, 0, rcClient.right, rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
                ReleaseDC(NULL, hdcScreen);
            }
#endif

            SelectObject(ptb->hdc, hbmOld);
            DeleteDC(ptb->hdc);
            //ReleaseDC(NULL, ptb->hdc);
            if (wParam == 0)
                EndPaint(hwnd, &ps);

            ptb->hdc = NULL;
            break;
        }

        case WM_GETDLGCODE:
            return DLGC_WANTARROWS;

        case WM_LBUTTONDOWN:
            /* Give ourselves focus */
            if (!(ptb->ci.style & WS_DISABLED)) {
                SetFocus(hwnd); // REVIEW: we may not want to do this
                TBTrackInit(ptb, lParam);
            }
            break;

        case WM_LBUTTONUP:
            // We're through doing whatever we were doing with the
            // button down.
            if (!(ptb->ci.style & WS_DISABLED)) {
                TBTrackEnd(ptb);
                if (GetCapture() == hwnd)
                    CCReleaseCapture(&ptb->ci);
            }
            break;

        case WM_TIMER:
            // The only way we get a timer message is if we're
            // autotracking.
            lParam = GetMessagePosClient(ptb->ci.hwnd, NULL);
            // fall through to WM_MOUSEMOVE

        case WM_MOUSEMOVE:
            // We only care that the mouse is moving if we're
            // tracking the bloody thing.
            if ((ptb->Cmd != (UINT)-1) && (!(ptb->ci.style & WS_DISABLED)))
                TBTrack(ptb, lParam);
            break;

        case WM_CAPTURECHANGED:
            // someone is stealing the capture from us
            TBTrackEnd(ptb);
            break;

        case WM_KEYUP:
            if (!(ptb->ci.style & WS_DISABLED)) {
                // If key was any of the keyboard accelerators, send end
                // track message when user up clicks on keyboard
                switch (wParam) {
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                    DoTrack(ptb, TB_ENDTRACK, 0);
                    break;
                default:
                    break;
                }
            }
            break;

        case WM_KEYDOWN:
            if (!(ptb->ci.style & WS_DISABLED)) {

                // Swap the left and right arrow key if the control is mirrored.
                wParam = RTLSwapLeftRightArrows(&ptb->ci, wParam);

                // If TBS_DOWNISLEFT, then swap left/right or up/down
                // depending on whether we are vertical or horizontal.
                // Some horizontal trackbars (e.g.) prefer that
                // UpArrow=TB_PAGEDOWN.
                if (ptb->ci.style & TBS_DOWNISLEFT) {
                    if (ISVERT(ptb)) {
                        wParam = CCSwapKeys(wParam, VK_LEFT, VK_RIGHT);
                    } else {
                        wParam = CCSwapKeys(wParam, VK_UP, VK_DOWN);
                        wParam = CCSwapKeys(wParam, VK_PRIOR, VK_NEXT);
                    }
                }

                switch (wParam) {
                case VK_HOME:
                    wParam = TB_TOP;
                    goto KeyTrack;

                case VK_END:
                    wParam = TB_BOTTOM;
                    goto KeyTrack;

                case VK_PRIOR:
                    wParam = TB_PAGEUP;
                    goto KeyTrack;

                case VK_NEXT:
                    wParam = TB_PAGEDOWN;
                    goto KeyTrack;

                case VK_LEFT:
                case VK_UP:
                    wParam = TB_LINEUP;
                    goto KeyTrack;

                case VK_RIGHT:
                case VK_DOWN:
                    wParam = TB_LINEDOWN;
                KeyTrack:
                    DoTrack(ptb, (int) wParam, 0);
#ifdef KEYBOARDCUES
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(ptb->ci), UISF_HIDEFOCUS);
#endif
                    break;

                default:
                    break;
                }
            }
            break;

        case WM_MBUTTONDOWN:
            SetFocus(hwnd);
            break;

        case WM_STYLECHANGED:
            if (wParam == GWL_STYLE) {
                ptb->ci.style = ((LPSTYLESTRUCT)lParam)->styleNew;
                TBResize(ptb);
            }
            break;

#ifdef KEYBOARDCUES
        case WM_UPDATEUISTATE:
        {
            DWORD dwUIStateMask = MAKEWPARAM(0xFFFF, UISF_HIDEFOCUS);

            if (CCOnUIState(&(ptb->ci), WM_UPDATEUISTATE, wParam & dwUIStateMask, lParam))
                InvalidateRect(hwnd, NULL, TRUE);

            goto DoDefault;
        }
#endif
        case TBM_GETPOS:
            return ptb->lLogPos;

        case TBM_GETSELSTART:
            return ptb->lSelStart;

        case TBM_GETSELEND:
            return ptb->lSelEnd;

        case TBM_GETRANGEMIN:
            return ptb->lLogMin;

        case TBM_GETRANGEMAX:
            return ptb->lLogMax;

        case TBM_GETPTICS:
            return (LRESULT)ptb->pTics;

        case TBM_CLEARSEL:
            ptb->Flags &= ~TBF_SELECTION;
            ptb->lSelStart = -1;
            ptb->lSelEnd   = -1;
            goto RedrawTB;

        case TBM_CLEARTICS:
            if (ptb->pTics)
                LocalFree((HLOCAL)ptb->pTics);

            ptb->pTics = NULL;
            ptb->nTics = 0;
            goto RedrawTB;

        case TBM_GETTIC:

            if (ptb->pTics == NULL || (int)wParam >= ptb->nTics)
                return -1L;

            return ptb->pTics[wParam];

        case TBM_GETTICPOS:

            if (ptb->pTics == NULL || (int)wParam >= ptb->nTics)
                return -1L;

            return TBLogToPhys(ptb,ptb->pTics[wParam]);

        case TBM_GETNUMTICS:
            if (ptb->ci.style & TBS_NOTICKS)
                return 0;

            if (ptb->ticFreq) {
                // first and last +
                return 2 + (ptb->nTics / ptb->ticFreq);
            }

            // if there's no ticFreq, then we fall down here.
            // 2 for the first and last tics that we always draw
            // when NOTICS isn't set.
            return 2;


        case TBM_SETTIC:
            /* not a valid position */
            if (((LONG)lParam) < ptb->lLogMin || ((LONG)lParam) > ptb->lLogMax)
                break;

            h = CCLocalReAlloc(ptb->pTics,
                                 sizeof(DWORD) * (ptb->nTics + 1));
            if (!h)
                return (LONG)FALSE;
            
            ptb->pTics = (PDWORD)h;
            ptb->pTics[ptb->nTics++] = (DWORD)lParam;

            TBInvalidateAll(ptb);
            return (LONG)TRUE;

        case TBM_SETTICFREQ:
            ptb->ticFreq = (int) wParam;
            DoAutoTics(ptb);
            goto RedrawTB;

        case TBM_SETPOS:
            /* Only redraw if it will physically move */
            if (wParam && TBLogToPhys(ptb, (DWORD) lParam) !=
                TBLogToPhys(ptb, ptb->lLogPos))
                MoveThumb(ptb, (DWORD) lParam);
            else
                ptb->lLogPos = BOUND((LONG)lParam,ptb->lLogMin,ptb->lLogMax);
            break;

        case TBM_SETSEL:

            if (!(ptb->ci.style & TBS_ENABLESELRANGE)) break;
            ptb->Flags |= TBF_SELECTION;

            if (((LONG)(SHORT)LOWORD(lParam)) < ptb->lLogMin)
                ptb->lSelStart = ptb->lLogMin;
            else
                ptb->lSelStart = (LONG)(SHORT)LOWORD(lParam);

            if (((LONG)(SHORT)HIWORD(lParam)) > ptb->lLogMax)
                ptb->lSelEnd = ptb->lLogMax;
            else
                ptb->lSelEnd   = (LONG)(SHORT)HIWORD(lParam);

            if (ptb->lSelEnd < ptb->lSelStart)
                ptb->lSelEnd = ptb->lSelStart;
            goto RedrawTB;

        case TBM_SETSELSTART:

            if (!(ptb->ci.style & TBS_ENABLESELRANGE)) break;
            ptb->Flags |= TBF_SELECTION;
            if (lParam < ptb->lLogMin)
                ptb->lSelStart = ptb->lLogMin;
            else
                ptb->lSelStart = (LONG) lParam;
            if (ptb->lSelEnd < ptb->lSelStart || ptb->lSelEnd == -1)
                ptb->lSelEnd = ptb->lSelStart;
            goto RedrawTB;

        case TBM_SETSELEND:

            if (!(ptb->ci.style & TBS_ENABLESELRANGE)) break;
            ptb->Flags |= TBF_SELECTION;
            if (lParam > ptb->lLogMax)
                ptb->lSelEnd = ptb->lLogMax;
            else
                ptb->lSelEnd = (LONG) lParam;
            if (ptb->lSelStart > ptb->lSelEnd || ptb->lSelStart == -1)
                ptb->lSelStart = ptb->lSelEnd;
            goto RedrawTB;

        case TBM_SETRANGE:

            ptb->lLogMin = (LONG)(SHORT)LOWORD(lParam);
            ptb->lLogMax = (LONG)(SHORT)HIWORD(lParam);
            if (ptb->lSelStart < ptb->lLogMin)
                ptb->lSelStart = ptb->lLogMin;
            if (ptb->lSelEnd > ptb->lLogMax)
                ptb->lSelEnd = ptb->lLogMax;
            DoAutoTics(ptb);
            goto RedrawTB;

        case TBM_SETRANGEMIN:
            ptb->lLogMin = (LONG)lParam;
            if (ptb->lSelStart < ptb->lLogMin)
                ptb->lSelStart = ptb->lLogMin;
            DoAutoTics(ptb);
            goto RedrawTB;

        case TBM_SETRANGEMAX:
            ptb->lLogMax = (LONG)lParam;
            if (ptb->lSelEnd > ptb->lLogMax)
                ptb->lSelEnd = ptb->lLogMax;
            DoAutoTics(ptb);

RedrawTB:
            ptb->lLogPos = BOUND(ptb->lLogPos, ptb->lLogMin,ptb->lLogMax);
            TBChanged(ptb, TBC_ALL);
            /* Only redraw if flag says so */
            if (wParam) {
                InvalidateRect(hwnd, NULL, FALSE);
                MoveThumb(ptb, ptb->lLogPos);
            }
            break;

        case TBM_SETTHUMBLENGTH:
            if (ptb->ci.style & TBS_FIXEDLENGTH) {
                ptb->iThumbHeight = (UINT)wParam;
                TBResize(ptb);
            }
            break;

        case TBM_GETTHUMBLENGTH:
            return ptb->iThumbHeight;

        case TBM_SETPAGESIZE: {
            LONG lOldPage = ptb->lPageSize == -1 ? (ptb->lLogMax - ptb->lLogMin)/5 : ptb->lPageSize;
            ptb->lPageSize = (LONG)lParam;
            return lOldPage;
        }

        case TBM_GETPAGESIZE:
            return ptb->lPageSize == -1 ? (ptb->lLogMax - ptb->lLogMin)/5 : ptb->lPageSize;

        case TBM_SETLINESIZE:  {
            LONG lOldLine = ptb->lLineSize;
            ptb->lLineSize = (LONG)lParam;
            return lOldLine;
        }

        case TBM_GETLINESIZE:
            return ptb->lLineSize;

        case TBM_GETTHUMBRECT:
            if (lParam) {
                *((LPRECT)lParam) = ptb->rcThumb;
                if (ISVERT(ptb)) FlipRect((LPRECT)lParam);
            }
            break;

        case TBM_GETTOOLTIPS:
            return (LRESULT)ptb->hwndToolTips;

        case TBM_SETTOOLTIPS:
            ptb->hwndToolTips = (HWND)wParam;
            break;

        case TBM_SETTIPSIDE:
        {
            UINT uOldSide = ptb->uTipSide;
            
            ptb->uTipSide = (UINT) wParam;
            return uOldSide;
        }

        case TBM_GETCHANNELRECT:
            GetChannelRect(ptb, (LPRECT)lParam);
            break;

        case TBM_SETBUDDY:
            return (LRESULT)TBSetBuddy(ptb, (BOOL)wParam, (HWND)lParam);

        case TBM_GETBUDDY:
            return (LRESULT)(wParam ? ptb->hwndBuddyLeft : ptb->hwndBuddyRight);

        case WM_GETOBJECT:
            if( lParam == OBJID_QUERYCLASSNAMEIDX )
                return MSAA_CLASSNAMEIDX_TRACKBAR;
            goto DoDefault;

        default:
            if (uMsg == g_msgMSWheel) {
                int   iWheelDelta;
                int   cDetants;
                long  lPos;
                ULONG ulPos;

                if (g_bRunOnNT || g_bRunOnMemphis)
                    iWheelDelta = (int)(short)HIWORD(wParam);
                else
                    iWheelDelta = (int)wParam;

                // Update count of scroll amount
                gcWheelDelta -= iWheelDelta;
                cDetants = gcWheelDelta / WHEEL_DELTA;
                if (cDetants != 0) {
                    gcWheelDelta %= WHEEL_DELTA;
                }

                if (g_bRunOnNT || g_bRunOnMemphis)
                {
                    if (wParam & (MK_SHIFT | MK_CONTROL))
                        goto DoDefault;
                }
                else
                {
                    if (GetKeyState(VK_SHIFT) < 0 || GetKeyState(VK_CONTROL) < 0)
                        goto DoDefault;
                }

                if (SHRT_MIN <= ptb->lLogPos && ptb->lLogPos <= SHRT_MAX) {
                    lPos = ptb->lLogPos + cDetants;
                    lPos = BOUND(lPos, ptb->lLogMin, ptb->lLogMax);
                    ulPos = BOUND(lPos, SHRT_MIN, SHRT_MAX);
                    if ((long) ulPos != ptb->lLogPos) {
                        MoveThumb(ptb, (long) ulPos);
                        DoTrack(ptb, TB_THUMBPOSITION, ulPos);
                    }
                }

                return TRUE;
            } else {
                LRESULT lres;
                if (CCWndProc(&ptb->ci, uMsg, wParam, lParam, &lres))
                    return lres;
            }

DoDefault:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0L;
}

/* DoTrack() */

void NEAR PASCAL DoTrack(PTRACKBAR ptb, int cmd, DWORD dwPos)
{
    LONG dpos;
    switch(cmd) {
        case TB_LINEDOWN:
            dpos = ptb->lLineSize;
            goto DMoveThumb;

        case TB_LINEUP:
            dpos = -ptb->lLineSize;
            goto DMoveThumb;

        case TB_PAGEUP:
        case TB_PAGEDOWN:
            if (ptb->lPageSize == -1) {
                dpos = (ptb->lLogMax - ptb->lLogMin) / 5;
                if (!dpos)
                    dpos = 1;
            } else {
                dpos = ptb->lPageSize;
            }

            if (cmd == TB_PAGEUP)
                dpos *= -1;

DMoveThumb: // move delta
            MoveThumb(ptb, ptb->lLogPos + dpos);
            break;

        case TB_BOTTOM:
            dpos = ptb->lLogMax; // the BOUND will take care of this;
            goto ABSMoveThumb;

        case TB_TOP:
            dpos = ptb->lLogMin; // the BOUND will take care of this;

ABSMoveThumb: // move absolute
            MoveThumb(ptb, dpos);
            break;

        default:  // do nothing
            break;

    }

    // BUGBUG:  for now, send both in vertical mode
    // note: we only send back a WORD worth of the position.
    if (ISVERT(ptb)) {
        FORWARD_WM_VSCROLL(ptb->ci.hwndParent, ptb->ci.hwnd, cmd, LOWORD(dwPos), SendMessage);
    } else
        FORWARD_WM_HSCROLL(ptb->ci.hwndParent, ptb->ci.hwnd, cmd, LOWORD(dwPos), SendMessage);
}

/* WTrackType() */

WORD NEAR PASCAL WTrackType(PTRACKBAR ptb, LONG lParam)
{
    POINT pt;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    if (ptb->Flags & TBF_NOTHUMB ||
        ptb->ci.style & TBS_NOTHUMB)            // If no thumb, just leave.
        return 0;

    if (ISVERT(ptb)) {
        // put point in virtual coordinates
        int temp;
        temp = pt.x;
        pt.x = pt.y;
        pt.y = temp;
    }

    if (PtInRect(&ptb->rcThumb, pt))
        return TB_THUMBTRACK;

    if (!PtInRect(&ptb->rc, pt))
        return 0;

    if (pt.x >= ptb->rcThumb.left)
        return TB_PAGEDOWN;
    else
        return TB_PAGEUP;
}

/* TBTrackInit() */

void NEAR PASCAL TBTrackInit(PTRACKBAR ptb, LPARAM lParam)
{
        WORD wCmd;

        if (ptb->Flags & TBF_NOTHUMB ||
            ptb->ci.style & TBS_NOTHUMB)         // No thumb:  just leave.
            return;

        wCmd = WTrackType(ptb, (LONG) lParam);
        if (!wCmd)
            return;

        SetCapture(ptb->ci.hwnd);

        ptb->Cmd = wCmd;
        ptb->dwDragPos = (DWORD)-1;

        // Set up for auto-track (if needed).
        if (wCmd != TB_THUMBTRACK) {
                // Set our timer up
                SetTimer(ptb->ci.hwnd, TIMER_ID, REPEATTIME, NULL);
        } else {
            int xPos;
            // thumb tracking...

            // store the offset between the cursor's position and the center of the thumb
            xPos = TBLogToPhys(ptb, ptb->lLogPos);
            ptb->dwDragOffset = (ISVERT(ptb) ? HIWORD(lParam) : LOWORD(lParam)) - xPos;

            if (ptb->hwndToolTips) {
                TOOLINFO ti;
                // don't bother setting the rect because we'll do it below
                // in FlushToolTipsMgr;
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_TRACK | TTF_CENTERTIP;
                ti.hwnd = ptb->ci.hwnd;
                ti.uId = (UINT_PTR)ptb->ci.hwnd;
                SendMessage(ptb->hwndToolTips, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
            }
        }

        TBTrack(ptb, lParam);
}

/* EndTrack() */

void NEAR PASCAL TBTrackEnd(PTRACKBAR ptb)
{
        // Decide how we're ending this thing.
        if (ptb->Cmd == TB_THUMBTRACK) {

            if (ptb->hwndToolTips)
                SendMessage(ptb->hwndToolTips, TTM_TRACKACTIVATE, (WPARAM)FALSE, 0);

            DoTrack(ptb, TB_THUMBPOSITION, ptb->dwDragPos);

        }

        KillTimer(ptb->ci.hwnd, TIMER_ID);

        // Always send TB_ENDTRACK message if there's some sort of command tracking.
        if (ptb->Cmd != (UINT)-1) {
            DoTrack(ptb, TB_ENDTRACK, 0);

            // Nothing going on.
            ptb->Cmd = (UINT)-1;
        }

        MoveThumb(ptb, ptb->lLogPos);
}

#define TBTS_RIGHTLEFT   1   // low bit means it's on the right or left

void NEAR PASCAL TBTrack(PTRACKBAR ptb, LPARAM lParam)
{
    DWORD dwPos;
    WORD pos;


    // See if we're tracking the thumb
    if (ptb->Cmd == TB_THUMBTRACK) {


        pos = (ISVERT(ptb)) ? HIWORD(lParam) : LOWORD(lParam);
        pos -= (WORD) ptb->dwDragOffset;
        dwPos = TBPhysToLog(ptb, (int)(SHORT)pos);

        // Tentative position changed -- notify the guy.
        if (dwPos != ptb->dwDragPos) {
            ptb->dwDragPos = dwPos;
            MoveThumb(ptb, dwPos);
            DoTrack(ptb, TB_THUMBTRACK, dwPos);
        }

        if (ptb->hwndToolTips) {
            RECT rc;
            POINT pt;
            int iPixel;
            UINT uTipSide = ptb->uTipSide;

            // find the center of the window
            GetClientRect(ptb->ci.hwnd, &rc);
            pt.x = rc.right / 2;
            pt.y = rc.bottom / 2;

            //find the position of the thumb
            iPixel = TBLogToPhys(ptb, dwPos);
            if (ISVERT(ptb)) {
                pt.y = iPixel;
                uTipSide |= TBTS_RIGHTLEFT;
            } else {
                pt.x = iPixel;
                uTipSide &= ~TBTS_RIGHTLEFT;
            }
            
            // move it out to the requested side
            switch (uTipSide) {

            case TBTS_TOP:
                pt.y = -1;
                break;

            case TBTS_LEFT:
                pt.x = -1;
                break;

            case TBTS_BOTTOM:
                pt.y = rc.bottom + 1;
                break;

            case TBTS_RIGHT:
                pt.x = rc.right + 1;
                break;
            }

            // map it to screen coordinates
            MapWindowPoints(ptb->ci.hwnd, HWND_DESKTOP, &pt, 1);

            SendMessage(ptb->hwndToolTips, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
        }

    }
    else {
        if (ptb->Cmd != WTrackType(ptb, (LONG) lParam))
            return;

        DoTrack(ptb, ptb->Cmd, 0);
    }
}




void NEAR PASCAL FlushChanges(PTRACKBAR ptb)
{
    HBRUSH hbr;
    NMCUSTOMDRAW nmcd;

#ifdef WIN32
    hbr = FORWARD_WM_CTLCOLORSTATIC(ptb->ci.hwndParent, ptb->hdc, ptb->ci.hwnd, SendMessage);
#else
    hbr = FORWARD_WM_CTLCOLOR(ptb->ci.hwndParent, ptb->hdc, ptb->ci.hwnd, CTLCOLOR_STATIC, SendMessage);
#endif

    if (hbr) {
        RECT rc;
        BOOL fClear = FALSE;

        if ( ptb->wDirtyFlags == TBC_ALL ) {
            GetClientRect(ptb->ci.hwnd, &rc);
            fClear = TRUE;
        } else if (ptb->wDirtyFlags & TBC_THUMB) {
            rc = ptb->rc;
            rc.left = 0;
            rc.right += ptb->iThumbWidth;
            if (ISVERT(ptb))
                FlipRect(&rc);
            fClear = TRUE;
        }
        if (fClear)
            FillRect(ptb->hdc, &rc, hbr);
    }

    nmcd.hdc = ptb->hdc;
    if (ptb->ci.hwnd == GetFocus())
        nmcd.uItemState = CDIS_FOCUS;
    else
        nmcd.uItemState = 0;

#ifdef KEYBOARDCUES
#if 0
    // BUGBUG: Custom draw stuff for UISTATE (stephstm)
    if (CCGetUIState(&(ptb->ci), KC_TBD))
        nmcd.uItemState |= CDIS_SHOWKEYBOARDCUES;
#endif
#endif
    nmcd.lItemlParam = 0;
    ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREPAINT, &nmcd);

    // for skip default, no other flags make sense..  only allow that one
    if (!(ptb->ci.dwCustom == CDRF_SKIPDEFAULT)) {
        DWORD dwRet = 0;
        // do the actual drawing

        if (nmcd.uItemState & CDIS_FOCUS)
        {
            DrawFocus(ptb, hbr);
        }

        nmcd.uItemState = 0;
        if (ptb->wDirtyFlags & TBC_TICS) {

            nmcd.dwItemSpec = TBCD_TICS;
            dwRet = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, &nmcd);

            if (!(dwRet == CDRF_SKIPDEFAULT)) {
                DrawTics(ptb);

                if (dwRet & CDRF_NOTIFYPOSTPAINT) {
                    nmcd.dwItemSpec = TBCD_TICS;
                    CICustomDrawNotify(&ptb->ci, CDDS_ITEMPOSTPAINT, &nmcd);
                }
            }
        }

        if (ptb->wDirtyFlags & TBC_THUMB) {


            // the channel
            GetChannelRect(ptb, &nmcd.rc);
            if (ISVERT(ptb))
                FlipRect(&nmcd.rc);
            nmcd.dwItemSpec = TBCD_CHANNEL;
            dwRet = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, &nmcd);

            if (!(dwRet == CDRF_SKIPDEFAULT)) {

                // flip it back from the last notify
                if (ISVERT(ptb))
                    FlipRect(&nmcd.rc);

                // the actual drawing
                DrawChannel(ptb, &nmcd.rc);

                if (dwRet & CDRF_NOTIFYPOSTPAINT) {

                    if (ISVERT(ptb))
                        FlipRect(&nmcd.rc);
                    nmcd.dwItemSpec = TBCD_CHANNEL;
                    CICustomDrawNotify(&ptb->ci, CDDS_ITEMPOSTPAINT, &nmcd);
                }
            }


            // the thumb
            nmcd.rc = ptb->rcThumb;
            if (ptb->Cmd == TB_THUMBTRACK) {
                nmcd.uItemState = CDIS_SELECTED;
            }

            if (ISVERT(ptb))
                FlipRect(&nmcd.rc);
            nmcd.dwItemSpec = TBCD_THUMB;
            dwRet = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, &nmcd);

            if (!(dwRet == CDRF_SKIPDEFAULT)) {

                if (ISVERT(ptb))
                    FlipRect(&nmcd.rc);

                // the actual drawing
                DrawThumb(ptb, &nmcd.rc, nmcd.uItemState & CDIS_SELECTED);

                if (dwRet & CDRF_NOTIFYPOSTPAINT) {
                    if (ISVERT(ptb))
                        FlipRect(&nmcd.rc);
                    nmcd.dwItemSpec = TBCD_THUMB;
                    CICustomDrawNotify(&ptb->ci, CDDS_ITEMPOSTPAINT, &nmcd);
                }
            }

        }
        ptb->wDirtyFlags = 0;

        // notify parent afterwards if they want us to
        if (ptb->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
            CICustomDrawNotify(&ptb->ci, CDDS_POSTPAINT, &nmcd);
        }
    }

#ifdef TB_DEBUG
    DebugMsg(DM_TRACE, TEXT("DrawDone"));
    {
        HDC hdcScreen;
        RECT rcClient;
        hdcScreen = GetDC(NULL);
        GetClientRect(ptb->ci.hwnd, &rcClient);
        BitBlt(hdcScreen, 200, 0, 200 + rcClient.right, rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
        ReleaseDC(NULL, hdcScreen);
    }
#endif

}
