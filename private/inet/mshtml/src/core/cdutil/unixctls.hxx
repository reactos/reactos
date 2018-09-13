
//+------------------------------------------------------------------------
//
//  File:       unixctls.cxx
//
//  Contents:   Drawing of Motif Controls in Trident
//
//  History:    
//
//  Note:
//
//         BUGBUG: We should change the Mainwin header files to export
//                 only what we need here for drawing.
//
//-------------------------------------------------------------------------

#include <mainwin.h>


typedef struct {
    int nCurPosition;   /* actual position * 100 */
    int nMinRange;      /* also *100 : nMinRange <= nCurPosition <= nMaxRange */
    int nMaxRange;      /* also *100 */ 
    int nButtonLength;	/* # Pixels along axis of scrollbar button.
			 * Usually GetSystemMetrics(SM_C{XH,YV}SCROLL)
			 * unless the scrollbar is too short in which
			 * case it is calculated.
			 * See MwComputeScrollButtonLength().
			 */
    int nThumbLength;	/* # Pixels along axis of thumb.  Either calculated
			 * (in MOTIF_LOOK) or fixed at (in WINDOWS_LOOK)
			 * at GetSystemMetrics(SM_C{XH,YV}THUMB).
			 * See MwComputeScrollButtonLength().
			 */
    int nButtonWidth;	/* # Pixels across axis of scrollbar button.
			 * Set in WM_CREATE and WM_SIZE messages.
			 */
    int nLength;	/* # Pixels along axis of scrollbar.
			 * Set in WM_CREATE and WM_SIZE messages.
			 */
    BOOL bDisabled;
    LONG lStyle;
    BOOL bRepainted;    /* used when tracking only, to see if a message
			 * caused a repaint
			 */
    HBRUSH hBrush;
    BOOL bLeftUpEnabled;
    BOOL bRightDownEnabled;
    BOOL	bShowArrows;
    int		nProcessingDirection,
		nHorizontalHeight,
		nVerticalWidth,
		nShadowThickness,
    		nTrackPos;
    UINT	nPage;
    DWORD	cBackground,
		cForeground,
		cSunny,
		cShadow,
		cTroughColor;
} ScrollBarInfo;

typedef enum {LeftTopButton,
		RightBottomButton,
		LeftTopThumbRect,
		RightBottomThumbRect,
		ThumbButton,
		AllScrollRect	/* Motif only */
		} eScrollBarArea;

extern "C"
void MwPaintMotifScrollRect(HDC hDC, eScrollBarArea eArea, LPRECT lpRect,
                            BOOL bPressed, ScrollBarInfo * infos);


