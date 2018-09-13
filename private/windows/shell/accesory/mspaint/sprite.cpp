
#include "stdafx.h"
#include "global.h"
#include "sprite.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
IMPLEMENT_DYNAMIC( CDragger, CObject )
IMPLEMENT_DYNAMIC( CMultiDragger, CDragger )
IMPLEMENT_DYNAMIC(CSprite, CDragger)
#endif

#include "memtrace.h"

extern BOOL moduleInit;

  /**********************************************************************/
  /*                      CDragger Implementation                       */
  /**********************************************************************/

/*
 * OPTIMIZATION
 *
 * At the moment, draggers get a new DC whenever they need to draw or
 * erase.  We could cut that in half easily by merging the draw/erase
 * code, and achieve even better wins by allocating a single DC for a
 * multiple selection draw/erase.
 */

CDragger::CDragger( CWnd* pWnd, CRect* pRect )
    {
    ASSERT(pWnd != NULL);

    m_pWnd  = pWnd;
    m_state = hidden;

    m_rect.SetRect(0,0,0,0);

    if (pRect != NULL)
        m_rect = *pRect;
    }

CDragger::~CDragger()
    {
    if (m_pWnd->m_hWnd != NULL && m_state != hidden)
        Hide();
    }

/* CDragger::Draw
 *
 * This is a specialized Draw to draw our drag rectangles; drag
 * rectangles are the dotted rectangles which we draw when the user is
 * dragging a tracker to move or resize a control.
 */
void CDragger::Draw()
    {
    ASSERT( m_pWnd != NULL );

    CRect rect = m_rect;

    /*
     * This gets complex -- hold on to your hat.  The m_rect is
     * measured in client coordinates of the window, but since we
     * need to use GetWindowDC rather than GetDC (to avoid having
     * the m_rect clipped by the dialog's children) we must map
     * these coordinates to window coords.  We do this by mapping
     * them into screen coordinates, computing the offset from the
     * upper left corner of the dialog's WindowRect, and mapping
     * them back.  It's the most efficient way I can think to do
     * it; other suggestions are welcome.
     */
    CRect parentRect;

    m_pWnd->GetWindowRect( &parentRect );
    m_pWnd->ClientToScreen( &rect );

    rect.OffsetRect( -parentRect.left, -parentRect.top );

    // now we've got "rect" in the coordinates of the thing we
    // want to draw on.

    int dx = (rect.right - rect.left) - 1;
    int dy = (rect.bottom - rect.top) - 1;

    CDC* dc = m_pWnd->GetWindowDC();

    ASSERT( dc != NULL );

    CBrush* oldBrush = dc->SelectObject( GetHalftoneBrush() );

    dc->PatBlt( rect.left     , rect.top       , dx, 1 , PATINVERT );
    dc->PatBlt( rect.left     , rect.bottom - 1, dx, 1 , PATINVERT );
    dc->PatBlt( rect.left     , rect.top       , 1 , dy, PATINVERT );
    dc->PatBlt( rect.right - 1, rect.top       , 1 , dy, PATINVERT );

    dc->SelectObject( oldBrush );

    m_pWnd->ReleaseDC( dc );
    }

/* CDragger::Erase
 *
 * Since the default draw uses XOR, we can just Draw again to erase!
 */
void CDragger::Erase()
    {
    Draw();
    }

/* CDragger::Show, Hide
 *
 * The "drag rectangle" is the dotted rectangle which we draw when the
 * user is moving or resizing a control by dragging it with the mouse.
 * These functions erase and draw the drag rectangle, respectively.
 */
void CDragger::Hide()
    {
    if (m_state != shown)
        return;

    m_state = hidden;
    Erase();
    }

void CDragger::Show()
    {
    if (m_state != hidden)
        return;

    m_state = shown;
    Draw();
    }


void CDragger::Obscure( BOOL bObscure )
    {
    if (bObscure)
        {
        if (m_state != shown)
            return;

        Hide();
        m_state = obscured;
        }
    else
        {
        if (m_state != obscured)
            return;

        m_state = hidden;
        Show();
        }
    }

/* CDragger::Move
 *
 * Since nearly every single occurance of "CDragger->Show" occurred in
 * the context "Hide, m_rect = foo, Show", I decided to merge this
 * functionality into a single C++ function.
 */
void CDragger::Move(const CRect& newRect, BOOL bForceShow)
    {
    if ((m_rect == newRect) && !bForceShow)
        return;

    BOOL fShow = bForceShow || m_state == shown;
    Hide();
    m_rect = newRect;

    if (fShow)
        Show();
    }

void CDragger::MoveBy(int cx, int cy, BOOL bForceShow)
    {
    CSize offset (cx, cy);
    CPoint newTopLeft = m_rect.TopLeft() + offset;
    Move(newTopLeft, bForceShow);
    }

CRect CDragger::GetRect() const
    {
    return m_rect;
    }

void CDragger::Move(const CPoint& newTopLeft, BOOL bForceShow)
    {
    Move(m_rect - m_rect.TopLeft() + newTopLeft, bForceShow);
    }

void CDragger::SetSize(const CSize& newSize, BOOL bForceShow)
    {
    CRect newRect  = m_rect;
    newRect.right  = newRect.left + newSize.cx;
    newRect.bottom = newRect.top  + newSize.cy;

    Move(newRect, bForceShow);
    }

CMultiDragger::CMultiDragger() : m_draggerList()
    {
    ASSERT( m_draggerList.IsEmpty() );
    }

CMultiDragger::CMultiDragger(CWnd *pWnd) : CDragger(pWnd), m_draggerList()
    {
    ASSERT(m_draggerList.IsEmpty());
    }


CMultiDragger::~CMultiDragger()
    {
    POSITION pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger *pDragger = (CDragger*) m_draggerList.GetNext(pos);
        delete pDragger;
        }
    }

CRect CMultiDragger::GetRect() const
    {
    // accumulate the bounding rectangle for the group
    POSITION pos = m_draggerList.GetHeadPosition();

    CRect boundRect (32767, 32767, -32767, -32767);
    while (pos != NULL)
        {
        CDragger    *pDragger = (CDragger*) m_draggerList.GetNext(pos);
        boundRect.left  = min (boundRect.left, pDragger->m_rect.left);
        boundRect.right = max (boundRect.right, pDragger->m_rect.right);
        boundRect.top   = min (boundRect.top, pDragger->m_rect.top);
        boundRect.bottom= max (boundRect.bottom, pDragger->m_rect.bottom);
        }

    return boundRect;
    }

void CMultiDragger::Hide()
    {
    // hide each dragger on the list
    POSITION pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        pDragger->Hide();
        }
    }

void CMultiDragger::Show()
    {
    // show each dragger on the list
    POSITION pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        pDragger->Show();
        }
    }

void CMultiDragger::Draw()
    {
    // draw each dragger on the list
    POSITION pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        pDragger->Draw();
        }
    }

void CMultiDragger::Erase()
    {
    // erase each dragger on the list
    POSITION pos = m_draggerList.GetHeadPosition();

    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        pDragger->Erase();
        }
    }

void CMultiDragger::Move(const CPoint& newTopLeft, BOOL bForceShow)
    {
    // move each dragger to the new top left

    // first go through the list and find the current topmost leftmost
    // point

    CPoint  topLeft (32767, 32767);
    POSITION pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        CRect   draggerRect = pDragger->GetRect();
        if (draggerRect.left < topLeft.x)
            topLeft.x= draggerRect.left;
        if (draggerRect.top < topLeft.y)
            topLeft.y= draggerRect.top;
        }

    // now find the offset and move each dragger
    CSize   offset = newTopLeft - topLeft;
    pos = m_draggerList.GetHeadPosition();
    while (pos != NULL)
        {
        CDragger* pDragger = (CDragger*) m_draggerList.GetNext(pos);
        pDragger->MoveBy(offset.cx, offset.cy, bForceShow);
        }
    }

void CMultiDragger::Add(CDragger *pDragger)
    {
    // add the dragger to the list
    ASSERT(pDragger != NULL);
    m_draggerList.AddTail(pDragger);
    }

void CMultiDragger::Remove(CDragger *pDragger)
    {
    // remove the dragger from the list
    ASSERT(pDragger != NULL);
    POSITION pos = m_draggerList.Find(pDragger);
    if (pos != NULL)
        m_draggerList.RemoveAt(pos);
    }


CSprite::CSprite() : m_saveBits()
    {
    m_state = hidden;
    m_pWnd = NULL;
    }


CSprite::CSprite(CWnd* pWnd, CRect* pRect)
        : CDragger(pWnd, pRect), m_saveBits()
    {
    m_state = hidden;
    m_pWnd = pWnd;
    }


CSprite::~CSprite()
    {
    if (m_pWnd->m_hWnd != NULL && m_state != hidden)
        Hide();
    }


void CSprite::Move(const CRect& newRect, BOOL bForceShow)
    {
    CRect rect = newRect;

    if ((rect == m_rect) && !bForceShow)
        return;

    STATE oldState = m_state;
    Hide();
    if (newRect.Size() != m_rect.Size())
        m_saveBits.DeleteObject();
    m_rect = rect;
    if (bForceShow || oldState == shown)
        Show();
    }

void CSprite::SaveBits()
    {
    CClientDC dcWnd(m_pWnd);
    CDC dcSave;
    CBitmap* pOldBitmap;

    dcSave.CreateCompatibleDC(&dcWnd);
    if (m_saveBits.m_hObject == NULL)
        {
        m_saveBits.CreateCompatibleBitmap(&dcWnd, m_rect.Width(),
            m_rect.Height());
        }
    pOldBitmap = dcSave.SelectObject(&m_saveBits);
    dcSave.BitBlt(0, 0, m_rect.Width(), m_rect.Height(),
        &dcWnd, m_rect.left, m_rect.top, SRCCOPY);
    dcSave.SelectObject(pOldBitmap);
    }


void CSprite::Erase()
    {
    if (m_saveBits.m_hObject == NULL)
        return;

    LONG dwStyle = ::GetWindowLong(m_pWnd->m_hWnd, GWL_STYLE);
    ::SetWindowLong(m_pWnd->m_hWnd, GWL_STYLE, dwStyle & ~WS_CLIPCHILDREN);

    CClientDC dcWnd(m_pWnd);
    CDC dcSave;
    CBitmap* pOldBitmap;

    dcSave.CreateCompatibleDC(&dcWnd);
    pOldBitmap = dcSave.SelectObject(&m_saveBits);
    dcWnd.ExcludeUpdateRgn(m_pWnd);
    dcWnd.BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
        &dcSave, 0, 0, SRCCOPY);
    dcSave.SelectObject(pOldBitmap);

    ::SetWindowLong(m_pWnd->m_hWnd, GWL_STYLE, dwStyle);
    }


CHighlight::CHighlight()
    {
    m_bdrSize = 2;
    }


CHighlight::CHighlight(CWnd *pWnd, CRect *pRect, int bdrSize)
           : CDragger(pWnd, pRect)
    {
    m_bdrSize = bdrSize;
    m_rect.InflateRect(m_bdrSize, m_bdrSize);
    }

CHighlight::~CHighlight()
    {
    if (m_pWnd->m_hWnd != NULL && m_state != hidden)
        Hide();
    }


void CHighlight::Draw()
    {
    m_pWnd->UpdateWindow();

    CClientDC   dc(m_pWnd);
    CBrush      *pOldBrush  = dc.SelectObject(GetSysBrush(COLOR_HIGHLIGHT));

    // draw the top, right, bottom and left sides
    dc.PatBlt(m_rect.left    + m_bdrSize, m_rect.top                 ,
              m_rect.Width() - m_bdrSize, m_bdrSize                  , PATCOPY);
    dc.PatBlt(m_rect.right - m_bdrSize  , m_rect.top + m_bdrSize     ,
              m_bdrSize                 , m_rect.Height() - m_bdrSize, PATCOPY);
    dc.PatBlt(m_rect.left               , m_rect.bottom - m_bdrSize  ,
              m_rect.Width() - m_bdrSize, m_bdrSize                  , PATCOPY);
    dc.PatBlt(m_rect.left               , m_rect.top                 ,
              m_bdrSize                 , m_rect.Height() - m_bdrSize, PATCOPY);

    // restore the state of the DC
    dc.SelectObject(pOldBrush);
    }

void CHighlight::Erase()
    {
    m_pWnd->InvalidateRect(&m_rect);
    m_pWnd->UpdateWindow();
    }
