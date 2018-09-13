
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "sprite.h"
#include "tracker.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

// FUTURE: Make these static to CTracker!
CBitmap NEAR g_bmapDragHandle;  // Handle for the drag handle bitmap.
CBitmap NEAR g_bmapDragHandle2; // Handle for hollow drag handle bitmap.


// These are the bitmaps arrays used for tracker borders and the dotted
// drag rectangles.
//
static unsigned short bmapHorizBorder[] =
                                { 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0 };

static unsigned short bmapVertBorder [] =
                                { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };

static CBrush  NEAR brushHorizBorder;
static CBrush  NEAR brushVertBorder;
static CBitmap NEAR bitmapHorizBorder;
static CBitmap NEAR bitmapVertBorder;

static HCURSOR hcurArrow    = NULL;     // System "Select Arrow" cursor.
static HCURSOR hcurMove     = NULL;     // System "Move" cursor.
static HCURSOR hcurSizeNESW = NULL;     // System sizing "NESW" cursor.
static HCURSOR hcurSizeNS   = NULL;     // System sizing "NS" cursor.
static HCURSOR hcurSizeNWSE = NULL;     // System sizing "NWSE" cursor.
static HCURSOR hcurSizeWE   = NULL;     // System sizing "WE" cursor.
static HCURSOR hcurDragTool;


// This array of hCursors is used to map tracker states (see the definition
// of CTracker in editor.hxx) to the appropriate mouse cursor bitmaps.
//
static HCURSOR* mapTrackerStateToPHCursor[] =
    {
    &hcurArrow,                             // nil
    &hcurArrow,                             // predrag
    &hcurMove,                              // moving
    &hcurSizeNS,                            // resizingTop
    &hcurSizeWE,                            // resizingLeft
    &hcurSizeWE,                            // resizingRight
    &hcurSizeNS,                            // resizingBottom
    &hcurSizeNWSE,                          // resizingTopLeft
    &hcurSizeNESW,                          // resizingTopRight
    &hcurSizeNESW,                          // resizingBottomLeft
    &hcurSizeNWSE,                          // resizingBottomRight
    };


HCURSOR HCursorFromTrackerState( int m )
    {
    ASSERT(m >= 0 &&
        m < sizeof (mapTrackerStateToPHCursor) / sizeof (HCURSOR*));
    return (*(mapTrackerStateToPHCursor[m]));
    }


/* RVUV2
 *
 * This code needs to be called, just once, before we begin to use
 * trackers.  In lieu of a standard initialization function into which
 * I can put this code, I am using a moduleInit variable as a kludge.
 */

BOOL moduleInit = FALSE;                    /**  RVUV2 temporary!  **/

BOOL InitTrackers()
    {
    /*
     * Initialize the brushes and bitmaps needed to do repaints
     */

    if (! bitmapHorizBorder.CreateBitmap( 8, 8, 1, 1, (LPSTR)bmapHorizBorder )
    ||  ! bitmapVertBorder.CreateBitmap ( 8, 8, 1, 1, (LPSTR)bmapVertBorder  )
    ||  ! brushHorizBorder.CreatePatternBrush( &bitmapHorizBorder )
    ||  ! brushVertBorder.CreatePatternBrush ( &bitmapVertBorder  )
    ||  ! g_bmapDragHandle.LoadBitmap ( IDBM_DRAGHANDLE )
    ||  ! g_bmapDragHandle2.LoadBitmap( IDBM_DRAGHANDLE2 ))
        {
        // Future: Failure here should cause error in opening dialog resource!
        theApp.SetMemoryEmergency( FALSE );
        return FALSE;
        }

    hcurArrow    = theApp.LoadStandardCursor( IDC_ARROW );
    hcurMove     = theApp.LoadCursor( IDCUR_MOVE     );
    hcurSizeNESW = theApp.LoadCursor( IDCUR_SIZENESW );
    hcurSizeNS   = theApp.LoadCursor( IDCUR_SIZENS   );
    hcurSizeNWSE = theApp.LoadCursor( IDCUR_SIZENWSE );
    hcurSizeWE   = theApp.LoadCursor( IDCUR_SIZEWE   );

    hcurDragTool = ::LoadCursor( AfxGetInstanceHandle(),
                                 MAKEINTRESOURCE( IDC_DRAGTOOL ));

    moduleInit = TRUE;

    return TRUE;
    }

/***************************************************************************/

void CTracker::CleanUpTracker()
    {
    brushHorizBorder.DeleteObject();
    brushVertBorder.DeleteObject();

    bitmapHorizBorder.DeleteObject();
    bitmapVertBorder.DeleteObject();

    g_bmapDragHandle.DeleteObject();
    g_bmapDragHandle2.DeleteObject();
    }

/***************************************************************************/
// NOTE: The rect passed in here is the inner-most rect of the tracker!

CTracker::STATE CTracker::HitTest( const CRect& rc,
                                         CPoint pt,
                                   STATE defaultState )
    {
    /*
     * Compute position of edge (non-corner) handles
     */
    int xMid = ((rc.right + rc.left) / 2) - (HANDLE_SIZE / 2);
    int yMid = ((rc.top + rc.bottom) / 2) - (HANDLE_SIZE / 2);

    /*
     * Now we do the actual hit-testing for each resizing handle
     */
    if ((pt.x < rc.left) && (pt.x > rc.left - HANDLE_SIZE))
        {
        if ((pt.y < rc.top) && (pt.y > rc.top - HANDLE_SIZE))
            return(resizingTopLeft);
        else
            if ((pt.y >= rc.bottom) && (pt.y < rc.bottom + HANDLE_SIZE))
                return(resizingBottomLeft);
            else
                if ( (pt.y >= yMid) && (pt.y < yMid + HANDLE_SIZE) )
                    return(resizingLeft);
        }
    else
        if ((pt.x >= rc.right) && (pt.x < rc.right + HANDLE_SIZE))
            {
            if ((pt.y < rc.top) && (pt.y > rc.top - HANDLE_SIZE))
                return(resizingTopRight);
            else
                if ((pt.y >= rc.bottom) && (pt.y < rc.bottom + HANDLE_SIZE))
                    return(resizingBottomRight);
                else
                    if ((pt.y >= yMid) && (pt.y < yMid + HANDLE_SIZE))
                        return(resizingRight);
            }
        else
            if ( (pt.x >= xMid) && (pt.x < xMid + HANDLE_SIZE) )
                {
                if ((pt.y < rc.top) && (pt.y > rc.top - HANDLE_SIZE))
                    return(resizingTop);
                else
                    if ((pt.y >= rc.bottom) && (pt.y < rc.bottom + HANDLE_SIZE))
                        return(resizingBottom);
                }

    return (defaultState);
    }

/******************************************************************************/

void CTracker::DrawBorder( CDC* dc, const CRect& trackerRect, EDGES edges )
    {
    if (! moduleInit)
        InitTrackers();     // RVUV2

    // Some precalculation for drawing the fuzzy borders
    int width       = trackerRect.Width();
    int height      = trackerRect.Height();
    int borderWidth =                             HANDLE_SIZE;
    int xLength     = width                     - HANDLE_SIZE * 2;
    int xHeight     = height                    - HANDLE_SIZE * 2;
    int xRight      = trackerRect.left + width  - HANDLE_SIZE;
    int yBottom     = trackerRect.top  + height - HANDLE_SIZE;
    int iOffset     = 1;

    // Draw the fuzzy borders.  Note that we have different bitmaps for
    // the vertical and horizontal borders.
    COLORREF windowColor    = GetSysColor( COLOR_WINDOW    );
    COLORREF highlightColor = GetSysColor( COLOR_HIGHLIGHT );

    dc->SetTextColor( windowColor    ); // colors reversed to adjust for
    dc->SetBkColor  ( highlightColor ); // patblt's reversed world view.

    CBrush* oldBrush = dc->SelectObject( &brushHorizBorder );

    if (! (edges & top))
        {
        dc->SelectObject( GetSysBrush( COLOR_APPWORKSPACE ) );
        iOffset = 0;
        }

    dc->PatBlt( trackerRect.left + HANDLE_SIZE, trackerRect.top + iOffset, xLength, borderWidth - 2 * iOffset, PATCOPY );
    dc->PatBlt( trackerRect.left + HANDLE_SIZE,         yBottom + iOffset, xLength, borderWidth - 2 * iOffset, PATCOPY );

    iOffset = 1;

//  dc->SelectObject( &brushVertBorder );

    if (! (edges & left))
        {
        dc->SelectObject( GetSysBrush( COLOR_APPWORKSPACE ) );
        iOffset = 0;
        }

    dc->PatBlt(           xRight + iOffset, trackerRect.top + HANDLE_SIZE, borderWidth - 2 * iOffset, xHeight, PATCOPY );
    dc->PatBlt( trackerRect.left + iOffset, trackerRect.top + HANDLE_SIZE, borderWidth - 2 * iOffset, xHeight, PATCOPY );

    dc->SelectObject( oldBrush );         // clean up
    }

/******************************************************************************/

void CTracker::DrawHandles( CDC* dc, const CRect& rect, EDGES edges )
    {
    /*
     * Some precalculation for tracker handles.  The bitmaps are colored,
     * but the function that loads them adds the windowColor and
     * selectionColor.
     */
    int x = rect.left + rect.Width() - HANDLE_SIZE;
    int y = rect.top + rect.Height() - HANDLE_SIZE;
    int xMid = rect.left + (((rect.Width() + 1) / 2) - (HANDLE_SIZE / 2));
    int yMid = rect.top + (((rect.Height() + 1) / 2) - (HANDLE_SIZE / 2));

    BOOL bTopLeft     = (edges & top   ) && (edges & left );
    BOOL bTopRight    = (edges & top   ) && (edges & right);
    BOOL bBottomLeft  = (edges & bottom) && (edges & left );
    BOOL bBottomRight = (edges & bottom) && (edges & right);
    /*
     * Choose a solid resizing handle if this is the currently selected
     * control, otherwise choose a hollow tracker handle.
     */
    CDC tempDC;

    if (!tempDC.CreateCompatibleDC(dc))
        {
        theApp.SetGdiEmergency();
        return;
        }
    /*
     * Draw the eight resizing handles.
     */
    dc->SetTextColor( GetSysColor( COLOR_HIGHLIGHT ) );
    dc->SetBkColor  ( GetSysColor( COLOR_WINDOW    ) );

    for (int i = 0; i < 2; i += 1)
        {
        CBitmap* pOldBitmap = tempDC.SelectObject( i? &g_bmapDragHandle2
                                                    : &g_bmapDragHandle );
        if (bTopLeft)
            dc->BitBlt(rect.left, rect.top, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (edges & top)
            dc->BitBlt(xMid, rect.top, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (bTopRight)
            dc->BitBlt(x, rect.top, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (edges & right)
            dc->BitBlt(x, yMid, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (bBottomRight)
            dc->BitBlt(x, y, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (edges & bottom)
            dc->BitBlt(xMid, y, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (bBottomLeft)
            dc->BitBlt(rect.left, y, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);
        if (edges & left)
            dc->BitBlt(rect.left, yMid, HANDLE_SIZE, HANDLE_SIZE,
                                             &tempDC, 0, 0, SRCCOPY);

        edges        = (EDGES)~(int)edges;
        bTopLeft     = !bTopLeft;
        bTopRight    = !bTopRight;
        bBottomLeft  = !bBottomLeft;
        bBottomRight = !bBottomRight;

        tempDC.SelectObject(pOldBitmap);
        }
    }

/******************************************************************************/

void CTracker::DrawBorderRgn( CDC* pdc, const CRect& trackerRect, CRgn *pcRgnPoly)
    {
    int ixOffset, iyOffset;

    if (! moduleInit)
        {
        InitTrackers(); // RVUV2
        }

    COLORREF windowColor    = GetSysColor( COLOR_WINDOW );
    COLORREF highlightColor = GetSysColor( COLOR_HIGHLIGHT );

    pdc->SetTextColor( windowColor    ); // colors reversed to adjust for
    pdc->SetBkColor  ( highlightColor ); // patblt's reversed world view.

    ixOffset = trackerRect.left + CTracker::HANDLE_SIZE + 1;
    iyOffset = trackerRect.top  + CTracker::HANDLE_SIZE + 1;

    // offset bitmap in the imgwnd from selection boundary
    if (pcRgnPoly                  != NULL
    &&  pcRgnPoly->GetSafeHandle() != NULL)
        {
        pcRgnPoly->OffsetRgn( ixOffset, iyOffset );

        pdc->FrameRgn( pcRgnPoly, &brushVertBorder, 1, 1 );

        pcRgnPoly->OffsetRgn( -ixOffset, -iyOffset );
        }
    }

/******************************************************************************/

void CTracker::DrawHandlesRgn( CDC* dc, const CRect& rect, EDGES edges, CRgn *pcRgnPoly)
    {
    /*
     * Some precalculation for tracker handles.  The bitmaps are colored,
     * but the function that loads them adds the windowColor and
     * selectionColor.
     */
    }

/******************************************************************************/

CTracker::STATE CTracker::HitTestRgn( const CRect& rc, CPoint pt,
                                   STATE defaultState, CRgn *pcRgnPoly)
    {
//  if (pcRgnPoly->PtInRegion(pt) != FALSE)

    return (defaultState);
    }

/******************************************************************************/
