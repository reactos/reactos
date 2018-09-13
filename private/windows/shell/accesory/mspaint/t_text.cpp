/******************************************************************************/
/* T_TEXT.CPP: IMPLEMENTATION OF THE CTextTool CLASS                        */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/*  CText Tool Class Object                                                   */
/*     CTextTool::CTextTool                                                   */
/*     CTextTool::~CTextTool                                                  */
/*     CTextTool::CreateTextEditObject                                        */
/*     CTextTool::PlaceTextOnBitmap                                           */
/*     CTextTool::OnUpdateColors                                              */
/*     CTextTool::OnCancel                                                    */
/*     CTextTool::OnStartDrag                                                 */
/*     CTextTool::OnEndDrag                                                   */
/*     CTextTool::OnDrag                                                      */
/*     CTextTool::OnClickOptions                                              */
/******************************************************************************/
/*                                                                            */
/* This is the Text edit tool.  It creates a tedit class object when the user */
/* is done dragging the selection for the size desired.                       */
/*                                                                            */
/* The Once a text object window exist, it is either cancelled or placed      */
/* according to the following rules.                                          */
/*                                                                            */
/* Cancel Rules                                                               */
/* - During a Drag, if the user drags more than MAX_MOVE_DIST_FOR_PLACE       */
/* - At the End of a Drag, if the user lets up the mouse more than            */
/*      MAX_MOVE_DIST_FOR_PLACE pixels from where they did the mosue down     */
/* - If the user selects anohter tool (in imgtools, select processing, see    */
/*      CImgTool::Select()).                                                  */
/*                                                                            */
/* Place Rules                                                                */
/* - At the End of a Drag, if the user lets up the mouse less than or equal   */
/*      to MAX_MOVE_DIST_FOR_PLACE pixels from where they did the mosue down  */
/*                                                                            */
/* Also, during the time the edit control object is visible/exists, the scroll*/
/* bars are disabled.                                                         */
/*                                                                            */
/******************************************************************************/

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "docking.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "pictures.h"
#include "tfont.h"
#include "tedit.h"
#include "t_Text.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CTextTool, CSelectTool )

#include "memtrace.h"

CTextTool NEAR g_TextTool;

/******************************************************************************/

CTextTool::CTextTool()
    {
    m_nCmdID         = IDMX_TEXTTOOL;
    m_pCTedit        = NULL;
    m_bIsUndoable    = TRUE;
    m_bCanBePrevTool = FALSE;
    }

/******************************************************************************/

CTextTool::~CTextTool()
    {
    }

/******************************************************************************/
/* Creates the CTedit class object with the appropriate attributes and        */
/* dissables the scroll bars on the bitmap window                             */

void CTextTool::CreateTextEditObject( CImgWnd* pImgWnd, MTI* pmti )
    {
    c_selectRect.SetRect( 0, 0, 0, 0 );

    if (pImgWnd         == NULL
    ||  pImgWnd->m_pImg == NULL)
        return;

    BOOL  bBackTransparent;
    CRect cRectTextBox;

    if (pmti->ptDown.x > pmti->pt.x)
        {
        cRectTextBox.left  = pmti->pt.x;
        cRectTextBox.right = pmti->ptDown.x;
        }
    else
        {
        cRectTextBox.left  = pmti->ptDown.x;
        cRectTextBox.right = pmti->pt.x;
        }

    if (pmti->ptDown.y > pmti->pt.y)
        {
        cRectTextBox.top    = pmti->pt.y;
        cRectTextBox.bottom = pmti->ptDown.y;
        }
    else
        {
        cRectTextBox.top    = pmti->ptDown.y;
        cRectTextBox.bottom = pmti->pt.y;
        }

    if (cRectTextBox.left   < 0)
        cRectTextBox.left   = 0;
    if (cRectTextBox.top    < 0)
        cRectTextBox.top    = 0;
    if (cRectTextBox.right  > pImgWnd->m_pImg->cxWidth  - 1)
        cRectTextBox.right  = pImgWnd->m_pImg->cxWidth  - 1;
    if (cRectTextBox.bottom > pImgWnd->m_pImg->cyHeight - 1)
        cRectTextBox.bottom = pImgWnd->m_pImg->cyHeight - 1;

    CRect rectImg;

    pImgWnd->GetClientRect( &rectImg );
    pImgWnd->ClientToImage(  rectImg );

    if (cRectTextBox.left   < rectImg.left)
        cRectTextBox.left   = rectImg.left;
    if (cRectTextBox.top    < rectImg.top )
        cRectTextBox.top    = rectImg.top;
    if (cRectTextBox.right  > rectImg.right)
        cRectTextBox.right  = rectImg.right - 1;
    if (cRectTextBox.bottom > rectImg.bottom)
        cRectTextBox.bottom = rectImg.bottom -1;

    bBackTransparent = ! theImgBrush.m_bOpaque;

    pImgWnd->ImageToClient( cRectTextBox );

    m_pCTedit = new CTedit;

    if (m_pCTedit != NULL
    &&  m_pCTedit->Create( pImgWnd, crLeft, crRight, cRectTextBox, bBackTransparent ))
        {
        SetupRubber( pImgWnd->m_pImg );

        pImgWnd->EnableScrollBar( SB_BOTH, ESB_DISABLE_BOTH );
        }
    else
        {
        TRACE( TEXT("Create Edit Window Failed!\n") );

        theApp.SetMemoryEmergency();
        }
    }

/******************************************************************************/
/* Places the image of the text edit control on the bitmap                    */
/* Then it deletes the text edit control, and re-enables the scroll bars      */

void CTextTool::PlaceTextOnBitmap( CImgWnd* pImgWnd )
    {
    if (m_pCTedit->IsModified())
        {
        CRect cRectClient;
        CDC*  pDC = CDC::FromHandle(pImgWnd->m_pImg->hDC);

        m_pCTedit->GetClientRect ( &cRectClient );
        m_pCTedit->ClientToScreen( &cRectClient );
        pImgWnd->ScreenToClient  ( &cRectClient );
        pImgWnd->ClientToImage   (  cRectClient );
        m_pCTedit->GetBitmap( pDC, &cRectClient );

        InvalImgRect ( pImgWnd->m_pImg, &cRectClient );
        CommitImgRect( pImgWnd->m_pImg, &cRectClient );

        pImgWnd->FinishUndo( cRectClient );

        DirtyImg( pImgWnd->m_pImg );
        }
    m_pCTedit->DestroyWindow();
    m_pCTedit = NULL;

    pImgWnd->EnableScrollBar( SB_BOTH, ESB_ENABLE_BOTH );
    }

/******************************************************************************/
/* updates the foreground and background colors                               */

void CTextTool::OnUpdateColors( CImgWnd* pImgWnd )
    {
    if (m_pCTedit != NULL)
        {
        m_pCTedit->SetTextColor( crLeft  );
        m_pCTedit->SetBackColor( crRight );
        }
    }

/******************************************************************************/

void CTextTool::OnActivate( BOOL bActivate )
    {
    if (bActivate)
        {
                // Disallow activation if Zoomed.
        if (CImgWnd::GetCurrent()->GetZoom() > 1 )
            {
            ::MessageBeep( MB_ICONASTERISK );

                        SelectPrevious();
            }
        }
    else
        {
        if (CWnd::GetCapture() != CImgWnd::c_pImgWndCur && m_pCTedit != NULL &&
                IsWindow(m_pCTedit->m_hWnd) )
            {
            CAttrEdit* pEdit = m_pCTedit->GetEditWindow();

            if (pEdit != NULL && IsWindow(pEdit->m_hWnd) && pEdit->GetWindowTextLength() > 0)
                PlaceTextOnBitmap( CImgWnd::c_pImgWndCur );
            else
                {
                m_pCTedit->DestroyWindow();
                m_pCTedit = NULL;
                InvalImgRect( CImgWnd::c_pImgWndCur->m_pImg, NULL ); // redraw selection

                CImgWnd::c_pImgWndCur->EnableScrollBar( SB_BOTH, ESB_ENABLE_BOTH );
                }
            }
        }
    CImgTool::OnActivate( bActivate );
    }

/******************************************************************************/
/* Deletes the text edit control, and refreshes the bitmap display, while     */
/* also re-enabling the scroll bars                                           */

void CTextTool::OnCancel(CImgWnd* pImgWnd)
    {
    if (m_pCTedit != NULL)
        {
        m_pCTedit->DestroyWindow();
        m_pCTedit = NULL;
        }

    InvalImgRect( pImgWnd->m_pImg, NULL );  // redraw selection

    pImgWnd->EnableScrollBar( SB_BOTH, ESB_ENABLE_BOTH );

    CImgTool::OnCancel( pImgWnd );
    }

/******************************************************************************/

void CTextTool::OnStartDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    CImgTool::OnStartDrag( pImgWnd, pmti );
    OnDrag( pImgWnd, pmti );
    }

/******************************************************************************/
/* if a text edit object does not exist, it creates one here.  If one does    */
/* exist, it checks the distance between the point down and point up.  If     */
/* less than or equal to MAX_MOVE_DIST_FOR_PLACE it places the bitmap, else   */
/* it assumes the user wants to abort the prior text editing session, and     */
/* destroys the prior text edit control and creates a new one with the newly  */
/* created dragged coordinate box (ptdown and ptup).                          */

void CTextTool::OnEndDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    CSize cPtDownUpDistance = pmti->ptDown - pmti->pt;

    // if the text box exists on a button up, was the button up close enough
    // to the button down to decide to place instead of throw away and
    // create a new text edit box.
    if (m_pCTedit != NULL)
        {
        PlaceTextOnBitmap( pImgWnd );

        int iDist = max( (abs( cPtDownUpDistance.cx )),
                         (abs( cPtDownUpDistance.cy )) );

        if (iDist <= MAX_MOVE_DIST_FOR_PLACE)
            {
            ClearStatusBarSize();
            CImgTool::OnEndDrag( pImgWnd, pmti );
            }
        else
            CreateTextEditObject( pImgWnd, pmti );
        }
    else // m_pCTedit == NULL either 1st time or destroyed, since on drag moved more than MAX_MOVE_DIS_FOR_PLACE
        {
        CreateTextEditObject( pImgWnd, pmti );
        }
    }

/******************************************************************************/

void CTextTool::OnDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    CPoint ptNew( pmti->pt.x, pmti->pt.y );
    CRect rectImg;

    pImgWnd->GetClientRect( &rectImg );
    pImgWnd->ClientToImage(  rectImg );

    if (! rectImg.PtInRect( ptNew ))
        {
        if (ptNew.x < rectImg.left)
            ptNew.x = rectImg.left;
        if (ptNew.x > rectImg.right)
            ptNew.x = rectImg.right;
        if (ptNew.y < rectImg.top)
            ptNew.y = rectImg.top;
        if (ptNew.y > rectImg.bottom)
            ptNew.y = rectImg.bottom;

        pmti->pt = ptNew;
        }
    CSelectTool::OnDrag( pImgWnd, pmti );
    }

/******************************************************************************/
/* Set the text edit tool  window's options for transparent or opaque         */

void CTextTool::OnClickOptions( CImgToolWnd* pWnd, const CRect& optionsRect,
                                                   const CPoint& clickPoint )
    {
    CSelectTool::OnClickOptions( pWnd, optionsRect, clickPoint );

    if (m_pCTedit != NULL)
        m_pCTedit->SetTransparentMode( ! theImgBrush.m_bOpaque );
    }

/******************************************************************************/
/* report to the rest of the program if the font palette is showin            */

BOOL CTextTool::FontPaletteVisible()
    {
    return (m_pCTedit? m_pCTedit->IsFontPaletteVisible(): FALSE);
    }

/******************************************************************************/
/* toggle the visable state of the Font Palette                               */

void CTextTool::ToggleFontPalette()
    {
    if (m_pCTedit)
        m_pCTedit->ShowFontPalette( m_pCTedit->IsFontPaletteVisible()? SW_HIDE: SW_SHOW );
    }

/******************************************************************************/

void CTextTool::OnShowControlBars(BOOL bShow)
{
        if (m_pCTedit == NULL)
        {
                return;
        }

        if (bShow)
        {
                if (!theApp.m_bShowTextToolbar)
                {
                        return;
                }

                m_pCTedit->ShowFontToolbar();
        }
        else
        {
                m_pCTedit->HideFontToolbar();
        }
}

/******************************************************************************/

void CTextTool::CloseTextTool( CImgWnd* pImgWnd )
    {
    if (! m_pCTedit)
        return;

        if ( IsWindow(pImgWnd->m_hWnd) )
                {
            if (! m_pCTedit->IsModified())
                {
                OnCancel( pImgWnd );
                return;
                }

            if (pRubberImg != pImgWnd->m_pImg)
                SetupRubber( pImgWnd->m_pImg );

                //  SetUndo( pImgWnd->m_pImg );

            PlaceTextOnBitmap( pImgWnd );

            pImgWnd->UpdateWindow();
                }
    }

/******************************************************************************/
