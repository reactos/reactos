/******************************************************************************/
/* Bar.CPP: IMPLEMENTATION OF THE CStatBar (Status Bar) CLASS                 */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/*    CStatBar::CStatBar                                                      */
/*    CStatBar::~CStatBar                                                     */
/*    CStatBar::Create                                                        */
/*    CStatBar::OnSetFont                                                     */
/*    CStatBar::DoPaint                                                       */
/*    CStatBar::DrawStatusText                                                */
/*    CStatBar::SetText                                                       */
/*    CStatBar::SetPosition                                                   */
/*    CStatBar::SetSize                                                       */
/*    CStatBar::ClearPosition                                                 */
/*    CStatBar::ClearSize                                                     */
/*    CStatBar::Reset                                                         */
/*    CStatBar::OnPaletteChanged                                                                                          */
/*                                                                            */
/*                                                                            */
/* Functions in this file                                                     */
/*                                                                            */
/*    ClearStatusBarSize                                                      */
/*    ClearStatusBarPosition                                                  */
/*    SetPrompt                                                               */
/*    SetPrompt                                                               */
/*    ShowStatusBar                                                           */
/*    IsStatusBarVisible                                                      */
/*    GetStatusBarHeight                                                      */
/*    InvalidateStatusBar                                                     */
/*    ClearStatusBarPositionAndSize                                           */
/*    ResetStatusBar                                                          */
/*    SetStatusBarPosition                                                    */
/*    SetStatusBarSize                                                        */
/*    SetStatusBarPositionAndSize                                             */
/*                                                                            */
/******************************************************************************/

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CStatBar, CStatusBar )

#include "memtrace.h"

CStatBar        *g_pStatBarWnd = NULL;

static UINT BASED_CODE indicators[] =
    {
    ID_SEPARATOR,           // status line indicator
    IDB_SBPOS,
    IDB_SBSIZE
    };

BEGIN_MESSAGE_MAP( CStatBar, CStatusBar )
                ON_WM_SYSCOLORCHANGE()
        ON_MESSAGE(WM_SETFONT, OnSetFont)
                ON_MESSAGE(WM_SIZEPARENT, OnSizeParent)
        ON_WM_NCDESTROY()
END_MESSAGE_MAP()

static int miSlackSpace;

/******************************************************************************/

CStatBar::CStatBar()
    {
    m_iBitmapWidth  = 0;
    m_iBitmapHeight = 0;
    miSlackSpace = 0;
    m_iSizeY = 0;
    }

CStatBar::~CStatBar()
        {
        // Ensure that the CControlBar doesn't assert trying access our parent
        // object (CPBFrame) during the destruction of our parent object.
        m_pDockSite = NULL;
        }

/******************************************************************************/

BOOL CStatBar::Create( CWnd* pParentWnd )
    {
    BOOL bRC = TRUE;
    int cxStatBar;  // width of a char in the status bar

    // Create the status Bar Window.
    bRC = CStatusBar::Create(pParentWnd);

    ASSERT (bRC != FALSE);

    if (bRC != FALSE)
        {
        // Set the Pane Indicators.
        bRC = SetIndicators( indicators, sizeof( indicators ) / sizeof( UINT ) );

        ASSERT( bRC != FALSE );

        if (bRC != FALSE)
            {
            TRY
                {
                // Load the Separator Strings
                VERIFY(m_cstringSizeSeparator.LoadString(IDS_SIZE_SEPARATOR));
                VERIFY(m_cstringPosSeparator.LoadString(IDS_POS_SEPARATOR));
                }
            CATCH(CMemoryException,e)
                {
                m_cstringSizeSeparator.Empty();
                m_cstringPosSeparator.Empty();
                }
            END_CATCH

            // Load the Position and Size Bitmaps
            VERIFY(m_posBitmap.LoadBitmap(IDB_SBPOS));
            VERIFY(m_sizeBitmap.LoadBitmap(IDB_SBSIZE));

            if ( (m_posBitmap.GetSafeHandle() != NULL) &&
                 (m_sizeBitmap.GetSafeHandle() != NULL)    )
                {
                //Calculate the size of the pane and set them

                CClientDC dc(this);

                /*DK* What font to select? */
                /*DK* What to do if in foreign Language, Size "0"? */

                cxStatBar = (dc.GetTextExtent(TEXT("0"), 1)).cx;
                BITMAP bmp;

                m_posBitmap.GetObject(sizeof (BITMAP), &bmp);

                m_iBitmapWidth  = bmp.bmWidth;
                m_iBitmapHeight = bmp.bmHeight;

                int iPaneWidth;
                UINT uiID, uiStyle;

                GetPaneInfo( 0, uiID, uiStyle, iPaneWidth) ;
                SetPaneInfo( 0, uiID, SBPS_NORMAL | SBPS_STRETCH, iPaneWidth );

                GetPaneInfo(1, uiID, uiStyle, iPaneWidth);

                if (iPaneWidth < bmp.bmWidth + (SIZE_POS_PANE_WIDTH * cxStatBar))
                    {
                    iPaneWidth = bmp.bmWidth + (SIZE_POS_PANE_WIDTH * cxStatBar);
                    SetPaneInfo(1, uiID, uiStyle, iPaneWidth);
                    }

                GetPaneInfo(2, uiID, uiStyle, iPaneWidth);

                if (iPaneWidth < bmp.bmWidth + (SIZE_POS_PANE_WIDTH * cxStatBar))
                    {
                    iPaneWidth = bmp.bmWidth + (SIZE_POS_PANE_WIDTH * cxStatBar);
                    SetPaneInfo(2, uiID, uiStyle, iPaneWidth);
                    }

                // force a height change
                CFont *pcFontTemp = GetFont();

                // initialize font height etc
                OnSetFont( (WPARAM)(HFONT)pcFontTemp->GetSafeHandle(), 0 );
                }
            else
                {
                bRC = FALSE;
                }
            }
        }
    return bRC;
    }

/******************************************************************************/

void CStatBar::OnNcDestroy( void )
    {
    m_posBitmap.DeleteObject();
    m_sizeBitmap.DeleteObject();

    m_posBitmap.m_hObject = NULL;
    m_sizeBitmap.m_hObject = NULL;

    m_cstringSizeSeparator.Empty();
    m_cstringPosSeparator.Empty();

    CStatusBar::OnNcDestroy();
    }

/******************************************************************************/

CSize CStatBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
    {
    CSize size = CStatusBar::CalcFixedLayout( bStretch, bHorz );

    size.cy = m_iSizeY;

    return size;
    }

/******************************************************************************/
/* Change the height of the status bar to allow the bitmaps to   be painted   */
/* in the panes. The height is set in the OnSetFont  OnSetFont method.  Save  */
/* the current border values, change  then call OnSetFont and then reset the  */
/* border values                                                              */
/*                                                                            */
/* This will increase the height of the whole status bar  until the next      */
/* OnSetFont (font change) for the status bar.                                */
/*                                                                            */
/* In Barcore.cpp, the Height of the status bar is set in the  OnSetFont      */
/* method as follows:                                                         */
/*                                                                            */
/*  Height =  = (tm.tmHeight - tm.tmInternalLeading) +                        */
/*              CY_BORDER*4 (which is 2 extra on top, 2                       */
/*              on bottom) - rectSize.Height();                               */
/*                                                                            */
/*  This is really                                                            */
/*    Height = Height of Font + Border between Font and                       */
/*             Pane edges + Border between Pane edges and                     */
/*             Status bar window.                                             */
/*                                                                            */
/*  tm.tmHeight - tm.tmInternalLeading is Font Height CY_BORDER*4 is border   */
/*  between font and pane edges rectSize.Height is Neg of Border between Pane */
/*  and SBar rectSize is set to 0, then the deflated by the border size.      */
/*  Deflating from 0 => negative, and  - negative gives us a positive amount. */
/*                                                                            */
/*  by default m_cyBottomBorder = m_cyTopBorder = 1                           */
/******************************************************************************/
/* We only change the border sizes temporarily for the calculation of the     */
/* status bar height.  We really don't want to change the border sizes, but   */
/* are just using them as a way to affect the size of the whole bar.          */
/******************************************************************************/

LRESULT CStatBar::OnSetFont(WPARAM wParam, LPARAM lParam)
    {

    CRect rect;

    int iTmpcyTopBorder    = m_cyTopBorder;
    int iTmpcyBottomBorder = m_cyBottomBorder;

    m_cyTopBorder = m_cyBottomBorder = 2;
    miSlackSpace = 0;

    // Can't do this in MFC 4
//    lResult = CStatusBar::OnSetFont(wParam, lParam); //initialize font height etc

    rect.SetRectEmpty();
    CalcInsideRect( rect, TRUE ); // will be negative size

    int iBorder = CY_BORDER * 4 - rect.Height();
    int iSize   = m_iSizeY - iBorder;
    int cyTallest = m_iBitmapHeight;
    CDC dc;

    if( dc.CreateIC( TEXT("DISPLAY"), NULL, NULL, NULL ) )
        {
        TEXTMETRIC tm;
                tm.tmHeight=0;
        CFont *font = CFont::FromHandle( (HFONT)wParam );
                if ( font )
                        {
                        CFont *oldFont = dc.SelectObject(font);

                if( dc.GetTextMetrics( &tm ) && tm.tmHeight > cyTallest )
                    cyTallest = tm.tmHeight;

                        if (oldFont)
                                dc.SelectObject(oldFont);
                        }
                dc.DeleteDC();
        }

    if (cyTallest > iSize)
        m_iSizeY     = cyTallest + iBorder;

    if (m_iBitmapHeight > iSize)
        miSlackSpace = m_iBitmapHeight - iSize;

    m_cyTopBorder    = iTmpcyTopBorder;
    m_cyBottomBorder = iTmpcyBottomBorder;

    return 1L;
    }

/******************************************************************************/
/* This routine is overloaded to allow us to paint the bitmaps in the panes.  */
/* If this routine was not here, it would work fine, but no bitmaps would     */
/* appear in the status indicator panes.                                      */
/******************************************************************************/

void CStatBar::DoPaint( CDC* pDC )
    {
    BOOL     bRC;
    CString  cstringText_Pane1;
    CString  cstringText_Pane2;
    CRect    rect_Pane1;
    CRect    rect_Pane2;
    CRgn     cRgn_Pane1;
    CRgn     cRgn_Pane2;
    CBitmap* pOldBitmap;
    UINT     uiStyle_Pane1;
    UINT     uiStyle_Pane2;
    UINT     uiID;
    int      iPaneWidth;
    HDC      hdc = pDC->GetSafeHdc();

    GetItemRect( 1, &rect_Pane1 );  // get pane rect
    GetItemRect( 2, &rect_Pane2 );  // get pane rect

    pDC->ExcludeClipRect( &rect_Pane1 ); // exclude pane rect from paint
    pDC->ExcludeClipRect( &rect_Pane2 ); // exclude pane rect from paint

    CStatusBar::DoPaint( pDC ); // Let Parent Class paint remainder of status bar

    CFont* pfntOld = pDC->SelectObject( GetFont() );

    GetPaneText( 1, cstringText_Pane1 );  // Get the Text for the Pane
    GetPaneText( 2, cstringText_Pane2 );  // The status bar holds the text for us.

    GetPaneInfo( 1, uiID, uiStyle_Pane1, iPaneWidth );
    GetPaneInfo( 2, uiID, uiStyle_Pane2, iPaneWidth );

    uiStyle_Pane1 = SBPS_NORMAL;
    uiStyle_Pane2 = SBPS_NORMAL;

    CDC srcDC; // select current bitmap into a compatible CDC

    bRC = srcDC.CreateCompatibleDC( pDC );

    ASSERT( bRC != FALSE );

    if (bRC != FALSE)
        {
        // Set the Text and Background Colors for a Mono to Color Bitmap
        // Conversion.  These are also set in DrawStatusText, so should not
        // have to reset them for the other bitmap/pane
        COLORREF crTextColor = pDC->SetTextColor( GetSysColor( COLOR_BTNTEXT ) );
        COLORREF crBkColor   = pDC->SetBkColor  ( GetSysColor( COLOR_BTNFACE ) );

        bRC = cRgn_Pane1.CreateRectRgnIndirect( rect_Pane1 );

        ASSERT( bRC != FALSE );

        if (bRC != FALSE)
            {
            pDC->SelectClipRgn( &cRgn_Pane1 ); // set clip region to pane rect

            pOldBitmap = srcDC.SelectObject( &m_posBitmap );

            rect_Pane1.InflateRect( -CX_BORDER, -CY_BORDER ); // deflate => don't paint on the borders

            pDC->BitBlt( rect_Pane1.left,    rect_Pane1.top,
                         rect_Pane1.Width(), rect_Pane1.Height(),
                         &srcDC, 0, 0, SRCCOPY ); // BitBlt to pane rect
            srcDC.SelectObject( pOldBitmap );

            rect_Pane1.InflateRect( CX_BORDER, CY_BORDER ); // Inflate back for drawstatustext

            // paint the borders and the text.
            DrawStatusText( hdc, rect_Pane1, cstringText_Pane1, uiStyle_Pane1,
                                                           m_iBitmapWidth + 1 );
            }

        cRgn_Pane2.CreateRectRgnIndirect(rect_Pane2);

        ASSERT( bRC != FALSE );

        if (bRC != FALSE)
            {
            pDC->SelectClipRgn(&cRgn_Pane2); // set clip region to pane rect

            pOldBitmap = srcDC.SelectObject(&m_sizeBitmap);
            rect_Pane2.InflateRect(-CX_BORDER, -CY_BORDER); // deflate => don't paint on the borders
            pDC->BitBlt(rect_Pane2.left, rect_Pane2.top, rect_Pane2.Width(),
                        rect_Pane2.Height(), &srcDC, 0, 0, SRCCOPY); // BitBlt to pane rect
            srcDC.SelectObject(pOldBitmap);
            rect_Pane2.InflateRect(CX_BORDER, CY_BORDER); // Inflate back for drawstatustext
            // DrawStatusText will paint the borders and the text.
            DrawStatusText(hdc, rect_Pane2, cstringText_Pane2, uiStyle_Pane2, m_iBitmapWidth+1);
            }
        pDC->SetTextColor( crTextColor );
        pDC->SetBkColor  ( crBkColor   );
        }
    if (pfntOld != NULL)
        pDC->SelectObject( pfntOld );
    }

/******************************************************************************/
/* Partially taken from BARCORE.CPP DrawStatusText method of CStatusBar.      */
/* Last parameter was added                                                   */
/*                                                                            */
/* This will allow us to output the text indented the space amount for our    */
/* bitmap.  Normally, this routine puts the text left alligned to the pane.   */
/******************************************************************************/

void PASCAL CStatBar::DrawStatusText( HDC    hDC,
                                      CRect const& rect,
                                      LPCTSTR lpszText,
                                      UINT   nStyle,
                                      int    iIndentText )
    {
    ASSERT(hDC != NULL);

    CBrush* cpBrushHilite;
    CBrush* cpBrushShadow;
    HBRUSH  hbrHilite = NULL;
    HBRUSH  hbrShadow = NULL;

    if (! (nStyle & SBPS_NOBORDERS))
        {
        if (nStyle & SBPS_POPOUT)
            {
            // reverse colors
            cpBrushHilite = GetSysBrush( COLOR_BTNSHADOW    );
            cpBrushShadow = GetSysBrush( COLOR_BTNHIGHLIGHT );
            }
        else
            {
            // normal colors
            cpBrushHilite = GetSysBrush( COLOR_BTNHIGHLIGHT );
            cpBrushShadow = GetSysBrush( COLOR_BTNSHADOW    );
            }

        hbrHilite = (HBRUSH)cpBrushHilite->GetSafeHandle();
        hbrShadow = (HBRUSH)cpBrushShadow->GetSafeHandle();
        }

    // background is already grey
    UINT nOpts           = ETO_CLIPPED;
    int nOldMode         = SetBkMode   ( hDC, OPAQUE );
    COLORREF crTextColor = SetTextColor( hDC, GetSysColor( COLOR_BTNTEXT ) );
    COLORREF crBkColor   = SetBkColor  ( hDC, GetSysColor( COLOR_BTNFACE ) );

    // Draw the hilites
    if (hbrHilite)
        {
        HGDIOBJ hOldBrush = SelectObject( hDC, hbrHilite );

        if (hOldBrush)
            {
            PatBlt( hDC, rect.right, rect.bottom, -(rect.Width() - CX_BORDER),
                                                        -CY_BORDER, PATCOPY );
            PatBlt( hDC, rect.right, rect.bottom, -CX_BORDER,
                                      -(rect.Height() - CY_BORDER), PATCOPY );
            SelectObject( hDC, hOldBrush );
            }
        }

    if (hbrShadow)
        {
        HGDIOBJ hOldBrush = SelectObject( hDC, hbrShadow );

        if (hOldBrush)
            {
            PatBlt( hDC, rect.left, rect.top, rect.Width(), CY_BORDER, PATCOPY );
            PatBlt( hDC, rect.left, rect.top,
                                   CX_BORDER, rect.Height(), PATCOPY );
            SelectObject( hDC, hOldBrush );
            }
        }

    // We need to adjust the rect for the ExtTextOut, and then adjust it back
    // just support left justified text
    if (lpszText != NULL && !(nStyle & SBPS_DISABLED))
        {
        CRect rectText( rect );

        rectText.InflateRect( -2 * CX_BORDER, -CY_BORDER );

        // adjust left edge for indented Text
        rectText.left += iIndentText;

            // align on bottom (since descent is more important than ascent)
        SetTextAlign( hDC, TA_LEFT | TA_BOTTOM );

        if (miSlackSpace > 0)
            rectText.InflateRect( 0, -(miSlackSpace / 2) );

        ExtTextOut( hDC, rectText.left, rectText.bottom,
                 nOpts, &rectText, lpszText, lstrlen( lpszText ), NULL );
        }

    SetTextColor( hDC, crTextColor );
    SetBkColor  ( hDC, crBkColor   );
    }

/******************************************************************************/

BOOL CStatBar::SetText(LPCTSTR szText)
    {
    BOOL bRC = TRUE;

    if (theApp.InEmergencyState())
        {
        bRC = FALSE;
        }
    else
        {
        bRC = SetPaneText(0, szText);
        }

    return bRC;
    }

/******************************************************************************/

BOOL CStatBar::SetPosition(const CPoint& pos)
    {
    BOOL bRC = TRUE;
    int cch;
    TCHAR szBuf [20];

    cch = wsprintf(szBuf, TEXT("%d~%d"), pos.x, pos.y);

    for (int i = 0; i < cch; i++)
        if (szBuf[i] == TEXT('~'))
            {
            szBuf[i] = m_cstringPosSeparator[0];
            break;
            }

    ASSERT (cch != 0);

    if (cch != 0)
        {
        bRC = SetPaneText(1, szBuf);
        }
    else
        {
        bRC = FALSE;
        }

    return bRC;
    }

/******************************************************************************/

BOOL CStatBar::SetSize(const CSize& size)
    {
    BOOL bRC = TRUE;
    int cch;
    TCHAR szBuf [20];

    cch = wsprintf( szBuf, TEXT("%d~%d"), size.cx, size.cy );

    for (int i = 0; i < cch; i++)
        if (szBuf[i] == TEXT('~'))
            {
            szBuf[i] = m_cstringSizeSeparator[0];
            break;
            }

    ASSERT (cch != 0);

    if (cch != 0)
        bRC = SetPaneText(2, szBuf);
    else
        bRC = FALSE;

    return bRC;
    }

/******************************************************************************/

BOOL CStatBar::ClearPosition()
    {
    BOOL bRC = TRUE;
    bRC = SetPaneText(1, TEXT(""));  // clear the position
    return bRC;
    }

/******************************************************************************/

BOOL CStatBar::ClearSize()
    {
    BOOL bRC = TRUE;
    bRC = SetPaneText(2, TEXT(""));  // clear the size
    return bRC;
    }


/******************************************************************************/

BOOL CStatBar::Reset()
    {
    return ClearPosition() && ClearSize();
    }

/******************************************************************************/

void CStatBar::OnSysColorChange()
        {
        CStatusBar::OnSysColorChange();
        InvalidateRect(NULL,FALSE);
        }

/******************************************************************************/

void ClearStatusBarSize()
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->ClearSize();
    }

/******************************************************************************/

void ClearStatusBarPosition()
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->ClearPosition();
    }

/******************************************************************************/

void SetPrompt(LPCTSTR szPrompt, BOOL bRedrawNow)
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->SetText(szPrompt);
    if (bRedrawNow)
        g_pStatBarWnd->UpdateWindow();
    }

/******************************************************************************/

void SetPrompt(UINT nStringID, BOOL bRedrawNow)
    {
        ASSERT(g_pStatBarWnd);
    CString str;
    VERIFY(str.LoadString(nStringID));

    g_pStatBarWnd->SetText(str);

    if (bRedrawNow)
        g_pStatBarWnd->UpdateWindow();
    }

/******************************************************************************/

void ShowStatusBar(BOOL bShow /* = TRUE */)
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->ShowWindow(bShow ? SW_SHOWNOACTIVATE : SW_HIDE);
    }

/******************************************************************************/

BOOL IsStatusBarVisible()
    {
        ASSERT(g_pStatBarWnd);
    return (g_pStatBarWnd->GetStyle() & WS_VISIBLE) != 0;
    }

/******************************************************************************/

int GetStatusBarHeight()
    {
        ASSERT(g_pStatBarWnd);
    CRect rect;
    g_pStatBarWnd->GetWindowRect(rect);
    return rect.Height();
    }

/******************************************************************************/

void InvalidateStatusBar(BOOL bErase /* = FALSE */)
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->Invalidate(bErase);
    }

/******************************************************************************/

void ClearStatusBarPositionAndSize()
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->ClearSize();
    g_pStatBarWnd->ClearPosition();
    }

/******************************************************************************/

void ResetStatusBar()
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->Reset();
    }

/******************************************************************************/

void SetStatusBarPosition(const CPoint& pos)
    {
        ASSERT(g_pStatBarWnd);
        if ( ::IsWindow(g_pStatBarWnd->m_hWnd) )
        g_pStatBarWnd->SetPosition(pos);
    }

/******************************************************************************/

void SetStatusBarSize(const CSize& size)
    {
        ASSERT(g_pStatBarWnd);
    if ( ::IsWindow( g_pStatBarWnd->m_hWnd) )
        g_pStatBarWnd->SetSize(size);
    }

/******************************************************************************/

void SetStatusBarPositionAndSize(const CRect& rect)
    {
        ASSERT(g_pStatBarWnd);
    g_pStatBarWnd->SetPosition(((CRect&)rect).TopLeft());
    g_pStatBarWnd->SetSize(rect.Size());
    }

/******************************************************************************/

LRESULT CStatBar::OnSizeParent(WPARAM wParam, LPARAM lParam)
{
        LRESULT lRes = CStatusBar::OnSizeParent(wParam, lParam);

        return(lRes);
}

