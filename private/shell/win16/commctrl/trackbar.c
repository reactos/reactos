//----------------------------------------------------------
//
// BUGBUG: make sure this stuff really works with the DWORD
//	   ranges
//
//----------------------------------------------------------

#include "ctlspriv.h"

//#define TB_DEBUG

typedef struct {
    HWND    hwnd;           // our window handle
    HWND    hwndParent;	    // sent notifys to
    HDC     hdc;            // current DC
    HBITMAP hbmBuffer;	    // double buffer
    UINT    wDirtyFlags;

    LONG    lLogMin;        // Logical minimum
    LONG    lLogMax;        // Logical maximum
    LONG    lLogPos;        // Logical position

    LONG    lSelStart;      // Logical selection start
    LONG    lSelEnd;        // Logical selection end

    int     iThumbWidth;    // Width of the thumb
    int     iThumbHeight;   // Height of the thumb

    int     iSizePhys;      // Size of where thumb lives
    RECT    rc;             // track bar rect.

    RECT    Thumb;          // Rectangle we current thumb
    DWORD   dwDragPos;      // Logical position of mouse while dragging.

    UINT    Flags;          // Flags for our window
    int     Timer;          // Our timer.
    UINT    Cmd;            // The command we're repeating.

    int     nTics;          // number of ticks.
    PDWORD  pTics;          // the tick marks.

    int     ticFreq;	    // the frequency of ticks
    LONG    style;	    // cache window style

    LONG     lPageSize;      // how much to thumb up and down.
    LONG     lLineSize;      // how muhc to scroll up and down on line up/down
#ifdef  FE_IME
    HIMC    hPrevImc;       // previous input context handle
#endif
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
void   NEAR PASCAL TBTrackInit(PTRACKBAR, LONG);
void   NEAR PASCAL TBTrackEnd(PTRACKBAR);
void   NEAR PASCAL TBTrack(PTRACKBAR, LONG);
void   NEAR PASCAL DrawThumb(PTRACKBAR);
HBRUSH NEAR PASCAL SelectColorObjects(PTRACKBAR, BOOL);
void   NEAR PASCAL SetTBCaretPos(PTRACKBAR);

#define TICKHEIGHT 3
#define BORDERSIZE 2

#define ABS(X)  (X >= 0) ? X : -X
#define BOUND(x,low,high)   max(min(x, high),low)

#define ISVERT(tb) (tb->style & TBS_VERT)

#define TBC_TICS 	0x1
#define TBC_THUMB 	0x2
#define TBC_ALL 	0xF

//
// Function Prototypes
//
LPARAM FAR CALLBACK TrackBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL TBChanged(PTRACKBAR tb, WORD wFlags );
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
#if !defined(WIN32) && !defined(IEWIN31_25)
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



/* flips the coordinates of the rect.
 * its here so that the same code can be used for horiz and vert
 * trackbars
 *
 * To add vertical capabilities, I'm using a virtual coordinate
 * system.  the ptb->Thumb and ptb->rc are in the virtual space (which
 * is just a horizontal trackbar).  Draw routines use PatRect and
 * TBBitBlt which switch to the real coordinate system as needed.
 *
 * The one gotcha is that the Thumb Bitmap has the pressed bitmap
 * to the real right, and the masks to the real right again for both
 * the vertical and horizontal Thumbs.  So those cases are hardcoded.
 * Do a search for ISVERT to find these dependancies.
 *				-Chee
 */
void NEAR PASCAL FlipRect(LPRECT prc)
{
    int temp;
    temp = prc->left;
    prc->left = prc->top;
    prc->top = temp;

    temp = prc->right;
    prc->right = prc->bottom;
    prc->bottom = temp;

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

void NEAR PASCAL TBInvalidateRect(HWND hwnd, LPRECT qrc, BOOL b, PTRACKBAR ptb)
{
    RECT rc;
    rc = *qrc;
    if (ISVERT(ptb)) FlipRect(&rc);
    InvalidateRect(hwnd, &rc, b);
}

void NEAR PASCAL TBDrawEdge(HDC hdc, LPRECT qrc, UINT edgeType, UINT grfFlags,
			    PTRACKBAR ptb)
{
    RECT temprc;
    UINT uFlags = grfFlags;

    temprc = *qrc;
    if (ISVERT(ptb)) {
	FlipRect(&temprc);

        if (!(uFlags & BF_DIAGONAL)) {
            if (grfFlags & BF_TOP) uFlags |= BF_LEFT;
            else uFlags &= ~BF_LEFT;

            if (grfFlags & BF_LEFT) uFlags |= BF_TOP;
            else uFlags &= ~BF_TOP;

            if (grfFlags & BF_RIGHT) uFlags |= BF_BOTTOM;
            else uFlags &= ~BF_BOTTOM;

            if (grfFlags & BF_BOTTOM) uFlags |= BF_RIGHT;
            else uFlags &= ~BF_RIGHT;
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

void NEAR PASCAL TBPatBlt(HDC hdc1, int x1, int y1, int w, int h,
			  DWORD rop, PTRACKBAR ptb)
{
    if (ISVERT(ptb))
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

/* DrawTics() */
/* There is always a tick at the beginning and end of the bar, but you can */
/* add some more of your own with a TBM_SETTIC message.  This draws them.  */
/* They are kept in an array whose handle is a window word.  The first     */
/* element is the number of extra ticks, and then the positions.           */

void NEAR PASCAL DrawTics(PTRACKBAR ptb)
{
    PDWORD pTics;
    int    iPos;
    int    yTic;
    int    i;
    int j, dir; //direction multiplier

    // do they even want this?
    if (ptb->style & TBS_NOTICKS) return;

    for( j = 0 ; j < 2; j++)  { // loop twice to not duplicate code..
	if (j == 0) { //first iteration do the bottom
	    if ((ptb->style & TBS_BOTH) || !(ptb->style & TBS_TOP)) {
		yTic = ptb->rc.bottom + 1;
		dir = 1;
	    } else {
		continue;
	    }

	} else { // second time through, do the top;
	    if ((ptb->style & (TBS_BOTH | TBS_TOP))) {
		yTic = ptb->rc.top - 1;
		dir = -1;
	    } else  {
		continue;
	    }
	}		

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
}

void NEAR PASCAL GetChannelRect(PTRACKBAR ptb, LPRECT lprc)
{
	int iwidth, iheight;
	lprc->left = ptb->rc.left - ptb->iThumbWidth / 2;
	iwidth = ptb->iSizePhys + ptb->iThumbWidth - 1;
	lprc->right = lprc->left + iwidth;
	
	if (ptb->style & TBS_ENABLESELRANGE) {
		iheight =  ptb->iThumbHeight / 4 * 3; // this is Scrollheight
	} else {
		iheight = 4;
	}

	lprc->top = (ptb->rc.top + ptb->rc.bottom - iheight) /2;
	if (!(ptb->style & TBS_BOTH))
	    if (ptb->style & TBS_TOP) lprc->top++;
	    else lprc->top--;
		
	lprc->bottom = lprc->top + iheight;

}

/* This draws the track bar itself */

void NEAR PASCAL DrawChannel(PTRACKBAR ptb)
{
        RECT rc;

        GetChannelRect(ptb, &rc);
	TBDrawEdge(ptb->hdc, &rc, EDGE_SUNKEN, BF_RECT,ptb);
	
	SetBkColor(ptb->hdc, g_clrBtnHighlight);
	// Fill the center
	PatRect(ptb->hdc, rc.left+2, rc.top+2, (rc.right-rc.left)-4,
                (rc.bottom-rc.top)-4, ptb);

		
	// now highlight the selection range
	if ((ptb->Flags & TBF_SELECTION) &&
	    (ptb->lSelStart <= ptb->lSelEnd) && (ptb->lSelEnd > ptb->lLogMin)) {
		int iStart, iEnd;

		iStart = TBLogToPhys(ptb,ptb->lSelStart);
		iEnd   = TBLogToPhys(ptb,ptb->lSelEnd);

		if (iStart + 2 <= iEnd) {
			SetBkColor(ptb->hdc, g_clrHighlight);
			PatRect(ptb->hdc, iStart+1, rc.top+3,
				iEnd-iStart-1, (rc.bottom-rc.top)-6, ptb);
		}
	}
}

void NEAR PASCAL DrawThumb(PTRACKBAR ptb)
{
    RECT rc;

    // iDpt direction from middle to point of thumb
    // a negative value inverts things.
    // this allows one code path..
    int iDpt;
    int i;	// size of point triangle
    int iYpt;	// vertical location of tip;
    int iXmiddle;
    int icount;  // just a loop counter
    UINT uEdgeFlags;

    if (ptb->Flags & TBF_NOTHUMB ||
        ptb->style & TBS_NOTHUMB)            // If no thumb, just leave.
        return;

    Assert(ptb->iThumbHeight >= MIN_THUMB_HEIGHT);
    Assert(ptb->iThumbWidth > 1);

    rc = ptb->Thumb;

    // draw the rectangle part
    if (!(ptb->style & TBS_BOTH))  {
    	int iMiddle;
        // do -3  because wThumb is odd (triangles ya know)
        // and because draw rects draw inside the rects passed.
        // actually should be (width-1)/2-1, but this is the same...

        i = (ptb->iThumbWidth - 3) / 2;
        iMiddle = ptb->iThumbHeight / 2 + ptb->Thumb.top;

        //draw the rectangle part
        if (ptb->style & TBS_TOP) {
    	    iMiddle++; //correction because drawing routines
    	    iDpt = -1;
    	    rc.top += (i+1);
    	    uEdgeFlags = BF_SOFT | BF_LEFT | BF_RIGHT | BF_BOTTOM;
        } else {
    	    iDpt = 1;
    	    rc.bottom -= (i+1);
    	    // draw on the inside, not on the bottom and rt edge
    	    uEdgeFlags = BF_SOFT | BF_LEFT | BF_RIGHT | BF_TOP;
        }

        iYpt = iMiddle + (iDpt * (ptb->iThumbHeight / 2));
        iXmiddle = rc.left + i;
    }  else {
        uEdgeFlags = BF_SOFT | BF_RECT;
    }

    // fill in the center
    if ((ptb->Cmd == TB_THUMBTRACK) || !IsWindowEnabled(ptb->hwnd)) {
    	HBRUSH hbrTemp;
    	// draw the dithered insides;
    	hbrTemp = SelectObject(ptb->hdc, g_hbrMonoDither);
    	if (hbrTemp) {
            SetTextColor(ptb->hdc, g_clrBtnHighlight);
            SetBkColor(ptb->hdc, g_clrBtnFace);
    	    TBPatBlt(ptb->hdc, rc.left +2 , rc.top,
    	    	 rc.right-rc.left -4, rc.bottom-rc.top,
    	    	 PATCOPY,ptb);

    	    if (!(ptb->style & TBS_BOTH)) {
    	
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
    	PatRect(ptb->hdc, rc.left+2, rc.top,
    		rc.right-rc.left-4, rc.bottom-rc.top, ptb);
    	
    	if (!(ptb->style & TBS_BOTH)) {
    	    for (icount = 1; icount <= i; icount++) {
    	    	PatRect(ptb->hdc, iXmiddle-icount+1,
    	    		iYpt - (iDpt*icount),
    	    		icount*2, 1, ptb);
    	    }	
    	}

    }

    TBDrawEdge(ptb->hdc, &rc, EDGE_RAISED, uEdgeFlags, ptb);


    //now draw the point
    if (!(ptb->style & TBS_BOTH)) {
        UINT uEdgeFlags2;

        // uEdgeFlags is now used to switch between top and bottom.
        // we'll or it in with the diagonal and left/right flags below
        if (ptb->style & TBS_TOP) {
            rc.bottom = rc.top + 1;
            rc.top = rc.bottom - (i + 2);
            if (ISVERT(ptb))
                uEdgeFlags = BF_TOP | BF_RIGHT | BF_DIAGONAL;
            else
                uEdgeFlags = BF_TOP | BF_RIGHT | BF_DIAGONAL | BF_SOFT;

            uEdgeFlags2 = BF_BOTTOM | BF_RIGHT | BF_DIAGONAL;
        } else {
            rc.top = rc.bottom - 1;
            rc.bottom = rc.top + (i + 2);

            // draw edge is not ssymetrical for this (and it can't be)
            if (ISVERT(ptb))
                uEdgeFlags = BF_TOP | BF_LEFT | BF_DIAGONAL;
            else
                uEdgeFlags = BF_TOP | BF_LEFT | BF_DIAGONAL | BF_SOFT;

            uEdgeFlags2 = BF_BOTTOM | BF_LEFT | BF_DIAGONAL;
        }

        rc.right = rc.left + (i + 2);
        // do the left side first
        TBDrawEdge(ptb->hdc, &rc, EDGE_RAISED, uEdgeFlags , ptb);
        // then do th right side
        OffsetRect(&rc, i + 1, 0);
        rc.right;
        TBDrawEdge(ptb->hdc, &rc, EDGE_RAISED, uEdgeFlags2 , ptb);
    }
}


void NEAR PASCAL TBInvalidateAll(PTRACKBAR ptb)
{
    if (ptb) {
        TBChanged(ptb, TBC_ALL);
        InvalidateRect(ptb->hwnd, NULL, FALSE);
    }
}

void NEAR PASCAL MoveThumb(PTRACKBAR ptb, LONG lPos)
{
    TBInvalidateRect(ptb->hwnd, &ptb->Thumb, FALSE,ptb);

    ptb->lLogPos  = BOUND(lPos,ptb->lLogMin,ptb->lLogMax);
    ptb->Thumb.left   = TBLogToPhys(ptb, ptb->lLogPos) - ptb->iThumbWidth / 2;
    ptb->Thumb.right  = ptb->Thumb.left + ptb->iThumbWidth;

    TBInvalidateRect(ptb->hwnd, &ptb->Thumb, FALSE,ptb);
    TBChanged(ptb, TBC_THUMB);
    UpdateWindow(ptb->hwnd);
}


void NEAR PASCAL DrawFocus(PTRACKBAR ptb)
{
    RECT rc;
    if (ptb->hwnd == GetFocus()) {
	SetBkColor(ptb->hdc, g_clrBtnHighlight);
	GetClientRect(ptb->hwnd, &rc);
	DrawFocusRect(ptb->hdc, &rc);
    }
}

void NEAR PASCAL DoAutoTics(PTRACKBAR ptb)
{
    LONG NEAR *pl;
    LONG l;

    if (!(ptb->style & TBS_AUTOTICKS))
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

    if (ptb->style & TBS_ENABLESELRANGE) {
        if (ptb->style & TBS_FIXEDLENGTH) {
            // half of 9/10
            ptb->iThumbWidth = (ptb->iThumbHeight * 9) / 20;
            ptb->iThumbWidth |= 0x01;
        } else {
            ptb->iThumbHeight += (ptb->iThumbWidth * 2) / 9;
        }
    }
}

void NEAR PASCAL TBResize(PTRACKBAR ptb)
{
    GetClientRect(ptb->hwnd, &ptb->rc);

    if (ISVERT(ptb))
        FlipRect(&ptb->rc);

    if (!(ptb->style & TBS_FIXEDLENGTH)) {
        ptb->iThumbHeight = (g_cyHScroll * 4) / 3;

        ValidateThumbHeight(ptb);
        if ((ptb->iThumbHeight > MIN_THUMB_HEIGHT) && (ptb->rc.bottom < (int)ptb->iThumbHeight)) {
            ptb->iThumbHeight = ptb->rc.bottom - 3*g_cyEdge; // top, bottom, and tic
            if (ptb->style & TBS_ENABLESELRANGE)
                ptb->iThumbHeight = (ptb->iThumbHeight * 3 / 4);
            ValidateThumbHeight(ptb);
        }
    } else {
        ValidateThumbHeight(ptb);
    }

    if (ptb->style & (TBS_BOTH | TBS_TOP) && !(ptb->style & TBS_NOTICKS))
        ptb->rc.top += TICKHEIGHT + BORDERSIZE + 3;
    ptb->rc.top	  += BORDERSIZE;
    ptb->rc.bottom  = ptb->rc.top + ptb->iThumbHeight;
    ptb->rc.left   += (ptb->iThumbWidth + BORDERSIZE);
    ptb->rc.right  -= (ptb->iThumbWidth + BORDERSIZE);

    ptb->Thumb.top = ptb->rc.top;
    ptb->Thumb.bottom = ptb->rc.bottom;

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

    if (ptb->hbmBuffer) {
        DeleteObject(ptb->hbmBuffer);
        ptb->hbmBuffer = NULL;
    }

    MoveThumb(ptb, ptb->lLogPos);
    TBInvalidateAll(ptb);
}

LPARAM FAR CALLBACK TrackBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PTRACKBAR       ptb;
	PAINTSTRUCT     ps;
	HLOCAL          h;

	ptb = (PTRACKBAR)GetWindowInt(hwnd, 0);

        switch (message) {

	case WM_CREATE:

	    #define lpCreate ((CREATESTRUCT FAR *)lParam)

            InitDitherBrush();
            InitGlobalMetrics(0);
            InitGlobalColors();

	    // Get us our window structure.
	    ptb = (PTRACKBAR)LocalAlloc(LPTR, sizeof(TRACKBAR));
            if (!ptb)
                return -1;

            SetWindowInt(hwnd, 0, (int)ptb);

	    ptb->hwnd = hwnd;
	    ptb->hwndParent = lpCreate->hwndParent;
	    ptb->style = lpCreate->style;
	    ptb->Cmd = (UINT)-1;
	    ptb->ticFreq = 1;
	    // ptb->hbmBuffer = 0;
            ptb->lPageSize = -1;
            ptb->lLineSize = 1;
            // initial size;
            ptb->iThumbHeight = (g_cyHScroll * 4) / 3;
#ifdef  FE_IME
            ptb->hPrevImc = ImmAssociateContext(hwnd, 0L);
#endif
            TBResize(ptb);
            break;

	case WM_WININICHANGE:

            InitGlobalMetrics(wParam);
	    // fall through to WM_SIZE

	case WM_SIZE:
            TBResize(ptb);
	    break;

        case WM_SYSCOLORCHANGE:
            ReInitGlobalColors();
            TBInvalidateAll(ptb);
            break;

	case WM_DESTROY:
            TerminateDitherBrush();
            if (ptb) {
#ifdef  FE_IME
            ImmAssociateContext(hwnd, ptb->hPrevImc);
#endif
                if (ptb->hbmBuffer)
                    DeleteObject(ptb->hbmBuffer);
                
                if (ptb->pTics)
                    LocalFree((HLOCAL)ptb->pTics);
                
	        LocalFree((HLOCAL)ptb);
                SetWindowInt(hwnd, 0, 0);
            }
	    break;

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
            if (ptb)
                TBInvalidateAll(ptb);
	    break;

	case WM_ENABLE:
            if (wParam) {
                ptb->style &= ~WS_DISABLED;
            } else {
                ptb->style |= WS_DISABLED;
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
            
            //DebugMsg(DM_TRACE, "NumTics = %d", SendMessage(ptb->hwnd, TBM_GETNUMTICS, 0, 0));
            
	    ptb->hdc = CreateCompatibleDC(hdc);
            if (!ptb->hbmBuffer) {
                GetClientRect(hwnd, &rc);
                ptb->hbmBuffer = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            }

	    hbmOld = SelectObject(ptb->hdc, ptb->hbmBuffer);
	    FlushChanges(ptb);
	
	    //only copy the area that's changable.. ie the clip box
	    switch(GetClipBox(hdc, &rc)) {
		case NULLREGION:
		case ERROR:
		    GetClientRect(ptb->hwnd, &rc);
	    }
	    BitBlt(hdc, rc.left, rc.top,
		     rc.right - rc.left, rc.bottom - rc.top,
		     ptb->hdc, rc.left, rc.top, SRCCOPY);

#ifdef TB_DEBUG
            {
                HDC hdcScreen;
                RECT rcClient;
                hdcScreen = GetDC(NULL);
                GetClientRect(ptb->hwnd, &rcClient);
                BitBlt(hdcScreen, 0, 0, rcClient.right, rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
                ReleaseDC(NULL, hdcScreen);
            }
#endif
	
	    SelectObject(ptb->hdc, hbmOld);
	    DeleteDC(ptb->hdc);
            if (wParam == 0)
	        EndPaint(hwnd, &ps);
	
	    ptb->hdc = NULL;
	    break;
	}	

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS;

	case WM_LBUTTONDOWN:
	    /* Give ourselves focus */
            if (!(ptb->style & WS_DISABLED)) {
                SetFocus(hwnd);	// REVIEW: we may not want to do this
                TBTrackInit(ptb, lParam);
            }
	    break;

	case WM_LBUTTONUP:
	    // We're through doing whatever we were doing with the
	    // button down.
            if (!(ptb->style & WS_DISABLED)) {
                TBTrackEnd(ptb);
                if (GetCapture() == hwnd)
                    ReleaseCapture();
            }
	    break;

	case WM_TIMER:
	    // The only way we get a timer message is if we're
	    // autotracking.
	    lParam = GetMessagePos();
#ifdef WIN32
            {
                RECT rc;
                rc.left = LOWORD(lParam);
                rc.top = HIWORD(lParam);
                ScreenToClient(ptb->hwnd, (LPPOINT)&rc);
                lParam = MAKELPARAM(rc.left, rc.top);
            }
#else
	    ScreenToClient(ptb->hwnd, (LPPOINT)&lParam);
#endif

	    // fall through to WM_MOUSEMOVE

	case WM_MOUSEMOVE:
	    // We only care that the mouse is moving if we're
	    // tracking the bloody thing.
	    if ((ptb->Cmd != (UINT)-1) && (!(ptb->style & WS_DISABLED)))
		TBTrack(ptb, lParam);
	    return 0L;

	case WM_CAPTURECHANGED:
	    // someone is stealing the capture from us
	    TBTrackEnd(ptb);
	    break;

	case WM_KEYUP:
            if (!(ptb->style & WS_DISABLED)) {
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
            if (!(ptb->style & WS_DISABLED)) {
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
		    DoTrack(ptb, wParam, 0);
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
                ptb->style = ((LPSTYLESTRUCT)lParam)->styleNew;
                TBResize(ptb);
            }
            return 0;

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
	    return (LONG)(LPVOID)ptb->pTics;

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
            if (ptb->style & TBS_NOTICKS)
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

	    if (ptb->pTics)
		h = LocalReAlloc((HLOCAL)ptb->pTics,
				 sizeof(DWORD) * (ptb->nTics + 1),
				 LMEM_MOVEABLE | LMEM_ZEROINIT);
	    else
		h = LocalAlloc(LPTR, sizeof(DWORD));

	    if (h)
		ptb->pTics = (PDWORD)h;
	    else
		return (LONG)FALSE;

	    ptb->pTics[ptb->nTics++] = (DWORD)lParam;

            TBInvalidateAll(ptb);
	    return (LONG)TRUE;
	
	case TBM_SETTICFREQ:
	    ptb->ticFreq = wParam;
	    DoAutoTics(ptb);
	    goto RedrawTB;

	case TBM_SETPOS:
	    /* Only redraw if it will physically move */
	    if (wParam && TBLogToPhys(ptb, lParam) !=
		TBLogToPhys(ptb, ptb->lLogPos))
		MoveThumb(ptb, lParam);
	    else
		ptb->lLogPos = BOUND(lParam,ptb->lLogMin,ptb->lLogMax);
	    break;

        case TBM_SETSEL:
	
	    if (!(ptb->style & TBS_ENABLESELRANGE)) break;
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

	    if (!(ptb->style & TBS_ENABLESELRANGE)) break;
	    ptb->Flags |= TBF_SELECTION;
            if (lParam < ptb->lLogMin)
                ptb->lSelStart = ptb->lLogMin;
            else
                ptb->lSelStart = lParam;
	    if (ptb->lSelEnd < ptb->lSelStart || ptb->lSelEnd == -1)
		ptb->lSelEnd = ptb->lSelStart;
	    goto RedrawTB;

	case TBM_SETSELEND:

	    if (!(ptb->style & TBS_ENABLESELRANGE)) break;
            ptb->Flags |= TBF_SELECTION;
            if (lParam > ptb->lLogMax)
                ptb->lSelEnd = ptb->lLogMax;
            else
                ptb->lSelEnd = lParam;
	    if (ptb->lSelStart > ptb->lSelEnd || ptb->lSelStart == -1)
		ptb->lSelStart = ptb->lSelEnd;
	    goto RedrawTB;

	case TBM_SETRANGE:

	    ptb->lLogMin = (LONG)(SHORT)LOWORD(lParam);
	    ptb->lLogMax = (LONG)(SHORT)HIWORD(lParam);
	    DoAutoTics(ptb);
	    goto RedrawTB;

	case TBM_SETRANGEMIN:
	    ptb->lLogMin = (LONG)lParam;
	    DoAutoTics(ptb);
	    goto RedrawTB;
	
	case TBM_SETRANGEMAX:
	    ptb->lLogMax = (LONG)lParam;
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
            if (ptb->style & TBS_FIXEDLENGTH) {
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
            *((LPRECT)lParam) = ptb->Thumb;
            if (ISVERT(ptb)) FlipRect((LPRECT)lParam);
            break;

        case TBM_GETCHANNELRECT:
            GetChannelRect(ptb, (LPRECT)lParam);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
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
	    goto DMoveThumb;
	
	case TB_BOTTOM:
	    dpos = ptb->lLogMax; // the BOUND will take care of this;
	    goto ABSMoveThumb;

	case TB_TOP:
	    dpos = ptb->lLogMin; // the BOUND will take care of this;
	    goto ABSMoveThumb;
	
DMoveThumb: // move delta
	    MoveThumb(ptb, ptb->lLogPos + dpos);
	    break;

ABSMoveThumb: // move absolute
	    MoveThumb(ptb, dpos);
	default:  // do nothing
	    break;
	
	
	
    }

    // BUGBUG:  for now, send both in vertical mode
    // note: we only send back a WORD worth of the position.
    if (ISVERT(ptb)) {
        FORWARD_WM_VSCROLL(ptb->hwndParent, ptb->hwnd, cmd, LOWORD(dwPos), SendMessage);
    } else
        FORWARD_WM_HSCROLL(ptb->hwndParent, ptb->hwnd, cmd, LOWORD(dwPos), SendMessage);
}

/* WTrackType() */

WORD NEAR PASCAL WTrackType(PTRACKBAR ptb, LONG lParam)
{
    POINT pt;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    if (ptb->Flags & TBF_NOTHUMB ||
        ptb->style & TBS_NOTHUMB)            // If no thumb, just leave.
	return 0;

    if (ISVERT(ptb)) {
	// put point in virtual coordinates
	int temp;
	temp = pt.x;
	pt.x = pt.y;
	pt.y = temp;
    }

    if (PtInRect(&ptb->Thumb, pt))
	return TB_THUMBTRACK;

    if (!PtInRect(&ptb->rc, pt))
	return 0;

    if (pt.x >= ptb->Thumb.left)
	return TB_PAGEDOWN;
    else
	return TB_PAGEUP;
}

/* TBTrackInit() */

void NEAR PASCAL TBTrackInit(PTRACKBAR ptb, LONG lParam)
{
	WORD wCmd;

    if (ptb->Flags & TBF_NOTHUMB ||
        ptb->style & TBS_NOTHUMB)         // No thumb:  just leave.
	    return;

        wCmd = WTrackType(ptb, lParam);
	if (!wCmd)
	    return;

	SetCapture(ptb->hwnd);

	ptb->Cmd = wCmd;
	ptb->dwDragPos = (DWORD)-1;

	// Set up for auto-track (if needed).
	if (wCmd != TB_THUMBTRACK) {
		// Set our timer up
		ptb->Timer = (UINT)SetTimer(ptb->hwnd, TIMER_ID, REPEATTIME, NULL);
	}

	TBTrack(ptb, lParam);
}

/* EndTrack() */

void NEAR PASCAL TBTrackEnd(PTRACKBAR ptb)
{
	// Decide how we're ending this thing.
	if (ptb->Cmd == TB_THUMBTRACK)
		DoTrack(ptb, TB_THUMBPOSITION, ptb->dwDragPos);

	if (ptb->Timer)
		KillTimer(ptb->hwnd, TIMER_ID);

	ptb->Timer = 0;

	// Always send TB_ENDTRACK message if there's some sort of command tracking.
        if (ptb->Cmd != (UINT)-1) {
            DoTrack(ptb, TB_ENDTRACK, 0);

            // Nothing going on.
            ptb->Cmd = (UINT)-1;
        }

	MoveThumb(ptb, ptb->lLogPos);
}

void NEAR PASCAL TBTrack(PTRACKBAR ptb, LONG lParam)
{
    DWORD dwPos;
    WORD pos;


    // See if we're tracking the thumb
    if (ptb->Cmd == TB_THUMBTRACK) {

	pos = (ISVERT(ptb)) ? HIWORD(lParam) : LOWORD(lParam);
	dwPos = TBPhysToLog(ptb, (int)(SHORT)pos);

	// Tentative position changed -- notify the guy.
	if (dwPos != ptb->dwDragPos) {
	    ptb->dwDragPos = dwPos;
	    MoveThumb(ptb, dwPos);
	    DoTrack(ptb, TB_THUMBTRACK, dwPos);
	}
    }
    else {
	if (ptb->Cmd != WTrackType(ptb, lParam))
	    return;

	DoTrack(ptb, ptb->Cmd, 0);
    }
}

// this is called internally when the trackbar has
// changed and we need to update the double buffer bitmap
// we only set a flag.  we do the actual draw
// during WM_PAINT.  This prevents wasted efforts drawing.
void NEAR PASCAL TBChanged(PTRACKBAR ptb, WORD wFlags)
{
    ptb->wDirtyFlags |= wFlags;
}

void NEAR PASCAL FlushChanges(PTRACKBAR ptb)
{
    HBRUSH hbr;

#ifdef WIN32
    hbr = FORWARD_WM_CTLCOLORSTATIC(ptb->hwndParent, ptb->hdc, ptb->hwnd, SendMessage);
#else
    hbr = FORWARD_WM_CTLCOLOR(ptb->hwndParent, ptb->hdc, ptb->hwnd, CTLCOLOR_STATIC, SendMessage);
#endif

#ifdef TB_DEBUG
            {
                HDC hdcScreen;
                RECT rcClient;
                hdcScreen = GetDC(NULL);
                GetClientRect(ptb->hwnd, &rcClient);
                BitBlt(hdcScreen, 0, 100, rcClient.right, 100 + rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
                ReleaseDC(NULL, hdcScreen);
            }
#endif

    if (hbr) {
	RECT rc;
        BOOL fClear = FALSE;

	if ( ptb->wDirtyFlags == TBC_ALL ) {
	    GetClientRect(ptb->hwnd, &rc);
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

#ifdef TB_DEBUG
            {
                HDC hdcScreen;
                RECT rcClient;
                hdcScreen = GetDC(NULL);
                GetClientRect(ptb->hwnd, &rcClient);
                BitBlt(hdcScreen, 0, 200, rcClient.right, 200 + rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
                ReleaseDC(NULL, hdcScreen);
            }
#endif
    if (ptb->wDirtyFlags & TBC_TICS) {
	DrawTics(ptb);
    }
#ifdef TB_DEBUG
            {
                HDC hdcScreen;
                RECT rcClient;
                hdcScreen = GetDC(NULL);
                GetClientRect(ptb->hwnd, &rcClient);
                BitBlt(hdcScreen, 200, 200, 200 + rcClient.right, 200 + rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
                ReleaseDC(NULL, hdcScreen);
            }
#endif

    if (ptb->wDirtyFlags & TBC_THUMB) {
	DrawChannel(ptb);
	DrawThumb(ptb);
    }
    if (ptb->wDirtyFlags & TBC_ALL) {
	DrawFocus(ptb);
    }
    ptb->wDirtyFlags = 0;

#ifdef TB_DEBUG
    DebugMsg(DM_TRACE, "DrawDone");
    {
        HDC hdcScreen;
        RECT rcClient;
        hdcScreen = GetDC(NULL);
        GetClientRect(ptb->hwnd, &rcClient);
        BitBlt(hdcScreen, 200, 0, 200 + rcClient.right, rcClient.bottom, ptb->hdc, 0,0, SRCCOPY);
        ReleaseDC(NULL, hdcScreen);
    }
#endif

}
