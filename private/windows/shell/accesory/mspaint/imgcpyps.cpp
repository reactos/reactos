
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgcolor.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "tedit.h"
#include "t_text.h"
#include "t_fhsel.h"
#include "toolbox.h"
#include "props.h"
#include "undo.h"
#include "srvritem.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

BOOL GetMFDimensions(
    HANDLE hMF,     /* handle to the CF_METAFILEPICT object from clipbrd */
    HDC hDC,        /* display context */
    long *pWidth,    /* width of picture in pixels, OUT param */
    long *pHeight,   /* height of picture in pixels, OUT param */
    IMG* pImg)
    ;
BOOL PlayMetafileIntoDC(
    HANDLE hMF,
    RECT *pRect,
    HDC hDC)
    ;

/***************************************************************************/

void CImgWnd::OnDestroyClipboard()
    {
    if (m_hPoints)
        {
        ::GlobalFree( m_hPoints );
        m_hPoints = NULL;
        }
    }

/***************************************************************************/

void CImgWnd::CopyBMAndPal(HBITMAP *pBM, CPalette ** ppPal)
    {
    IMG* pImg = m_pImg;

    CRect copyRect;

    if (theImgBrush.m_pImg == NULL)
        {
        HideBrush();
        copyRect.SetRect(0, 0, pImg->cxWidth, pImg->cyHeight);
        }
    else
        {
        copyRect = rcDragBrush;
        copyRect.right  -= 1;
        copyRect.bottom -= 1;
        }

    BOOL bRegion = (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL);

#ifdef FHSELCLIP
    if (bRegion)
        {
        if (! m_wClipboardFormat)
            m_wClipboardFormat = RegisterClipboardFormat( TEXT("MSPaintFreehand") );

        if (theImgBrush.m_bFirstDrag)
//          PickupSelection(); but no way to tell if we do it twice...
            PrepareForBrushChange( TRUE, FALSE );

        CFreehandSelectTool* pTool = (CFreehandSelectTool*)CImgTool::GetCurrent();

        ASSERT( pTool );

        if (m_wClipboardFormat && pTool)
            {
            CPoint* pptArray;
            int     iEntries;
            BOOL    bData = pTool->CopyPointsToMemArray( &pptArray, &iEntries );

            if (bData && iEntries)
                {
                HGLOBAL hMem = ::GlobalAlloc( GHND | GMEM_MOVEABLE | GMEM_DDESHARE,
                                                    iEntries * sizeof( POINT )
                                                             + sizeof( short ));
                if (hMem)
                    {
                    short* pShort = (short*)::GlobalLock( hMem );

                    *pShort++ = iEntries;

                    LPPOINT pPts = (LPPOINT)pShort;

                    for (int iPt = 0; iPt < iEntries; iPt++, pPts++)
                        {
                        pPts->x = pptArray[iPt].x - pTool->m_cRectBounding.left;
                        pPts->y = pptArray[iPt].y - pTool->m_cRectBounding.top;
                        }

                    ::GlobalUnlock( hMem );

                    if (m_hPoints)
                        {
                        ::GlobalFree( m_hPoints );
                        m_hPoints = NULL;
                        }

                    m_hPoints = SetClipboardData( m_wClipboardFormat, hMem );
                    }
                else
                    theApp.SetMemoryEmergency();

                delete [] pptArray;
                }
            }
        else
            theApp.SetGdiEmergency();
        }
#endif // FHSELCLIP

    if ( theImgBrush.m_pImg )
        {
        CPalette* ppalOld = SetImgPalette( &theImgBrush.m_dc );

        // Copy the selection...
        CRect rc( 0, 0, theImgBrush.m_size.cx, theImgBrush.m_size.cy );

        *pBM = CopyDC( &theImgBrush.m_dc, &rc );

        if (ppalOld)
            theImgBrush.m_dc.SelectPalette( ppalOld, TRUE );
        }
    else
        // Copy the whole image...
        *pBM = CopyDC( CDC::FromHandle( m_pImg->hDC ), &copyRect );

    if (theApp.m_pPalette && (*ppPal=new CPalette)!=NULL)
        {
        LOGPALETTE256 logPal;

        logPal.palVersion = 0x300;
        logPal.palNumEntries = (WORD)theApp.m_pPalette->GetPaletteEntries( 0, 256,
                                                     &logPal.palPalEntry[0]);

                if ( logPal.palNumEntries )
                        {
                theApp.m_pPalette->GetPaletteEntries( 0, logPal.palNumEntries,
                                                             &logPal.palPalEntry[0] );

                (*ppPal)->CreatePalette( (LPLOGPALETTE)&logPal );
                        }
        }
    }

void CImgWnd::CmdCopy()
{
        if (TextToolProcessed( ID_EDIT_COPY ))
        {
                return;
        }

        CBitmapObj* pResObject = new CBitmapObj;
        if (pResObject)
        {
                IMG* pImgStruct = new IMG;

                if (pImgStruct)
                {
                        if (FillBitmapObj(c_pImgWndCur, pResObject, pImgStruct))
                        {
                                pImgStruct->m_pFirstImgWnd = NULL;
                                pImgStruct->m_pBitmapObj = pResObject;

                                HDC hDCSave = pImgStruct->hDC;

                                pImgStruct->hDC = NULL;
                                pImgStruct->hMaskDC = NULL;

                                pImgStruct->hMaskBitmap = NULL;
                                pImgStruct->hMaskBitmapOld = NULL;

                                pImgStruct->hBitmap = NULL;
                                pImgStruct->m_pPalette = NULL;
                                CopyBMAndPal(&pImgStruct->hBitmap, &pImgStruct->m_pPalette);

                                if (pImgStruct->hBitmap)
                                {
                                        pImgStruct->hDC = CreateCompatibleDC(hDCSave);
                                        if (pImgStruct->hDC)
                                        {
                                                pImgStruct->hBitmapOld = (HBITMAP)SelectObject(
                                                        pImgStruct->hDC, pImgStruct->hBitmap);
                                                pImgStruct->m_hPalOld = pImgStruct->m_pPalette
                                                        ? SelectPalette(pImgStruct->hDC,
                                                        (HPALETTE)pImgStruct->m_pPalette->m_hObject, FALSE)
                                                        : NULL;

                                                // get a server item suitable to generate the clipboard data
                                                CPBView* pView = (CPBView*)
                                                        ((CFrameWnd*)AfxGetMainWnd())->GetActiveView();
                                                CPBSrvrItem* pItem = new CPBSrvrItem(pView->GetDocument(),
                                                        pResObject);

                                                if (pItem)
                                                {
                                                        pItem->CopyToClipboard(FALSE);

                                                        return;
                                                }
                                        }
                                }
                        }
                        else
                        {
                                // the IMG and all it contains will get cleaned up when
                                // pResObject is deleted, but only if FillBitmapObj succeeded
                                delete pImgStruct;
                        }
                }

                delete pResObject;
        }
}

/***************************************************************************/

void CImgWnd::CmdCut()
    {
    if (TextToolProcessed( ID_EDIT_CUT ))
        return;

    // BOGUS:
    // CmdCopy doesn't just copy -- it can change the state of the selection
    // this forces the CmdClear to act in the context of the new state
    // save off a silly flag for CmdClear to special-case like 'first-drag'
    BOOL *pFlag;
    if (theImgBrush.m_pImg && theImgBrush.m_bFirstDrag)
        {
        pFlag = &theImgBrush.m_bCuttingFromImage;
        }
    else
        pFlag = NULL;

    CmdCopy();

    TRY
        {
        if (pFlag)
            *pFlag = TRUE;

        CmdClear();
        }
    CATCH_ALL(e)
        {
        // don't leave the flag set
        if (pFlag)
            *pFlag = FALSE;

        THROW_LAST();
        }
    END_CATCH_ALL

    // normal execution path
    if (pFlag)
        *pFlag = FALSE;
    }

/***************************************************************************/

void CImgWnd::CmdPaste()
    {
    if (TextToolProcessed( ID_EDIT_PASTE ))
        return;

    CancelToolMode(FALSE);

    CommitSelection(TRUE);

    HideBrush();
    SetupRubber( m_pImg );
    EraseTracker();
    theImgBrush.m_pImg = NULL;
    DrawTracker();
    SetUndo( m_pImg );

    if (! PasteImageClip())
        AfxMessageBox( IDS_ERROR_CLIPBOARD, MB_OK | MB_ICONHAND );
    }

/***************************************************************************/

HBITMAP CImgWnd::CopyDC( CDC* pImgDC, CRect* prcClip )
    {
    // BLOCK: copy the image to hStdBitmap for the clipboard
    CDC       dc;
    CBitmap   bm;
    CBitmap*  pOldStdBitmap;
    int       cxWidth  = prcClip->Width();
    int       cyHeight = prcClip->Height();

    if (! dc.CreateCompatibleDC    ( pImgDC                    )
    ||  ! bm.CreateCompatibleBitmap( pImgDC, cxWidth, cyHeight ))
        {
        theApp.SetGdiEmergency();
        return FALSE;
        }

    pOldStdBitmap = dc.SelectObject( &bm );

    CPalette* pOldPalette = SetImgPalette( &dc );

    dc.BitBlt( 0, 0, cxWidth, cyHeight, pImgDC, prcClip->left, prcClip->top, SRCCOPY );
    dc.SelectObject( pOldStdBitmap );

    if (pOldPalette)
        dc.SelectPalette( pOldPalette, FALSE );

    // return the standard format (bitmap) data
    return (HBITMAP)bm.Detach();
    }

/***************************************************************************/

BOOL CImgWnd::IsPasteAvailable()
    {
    BOOL bPasteIsAvailable = FALSE;
    BOOL bBitmapAvailable  = IsClipboardFormatAvailable( CF_BITMAP );
    BOOL bDIBAvailable     = IsClipboardFormatAvailable( CF_DIB );
    BOOL bTextAvailable    = IsClipboardFormatAvailable( CF_TEXT );
    BOOL bMFAvailable      = IsClipboardFormatAvailable( CF_METAFILEPICT );

    if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
        CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();

        if (pTextTool                     != NULL
        &&  pTextTool->GetTextEditField() != NULL)
            bPasteIsAvailable = bTextAvailable;
        }
    else
        {
        bPasteIsAvailable = bBitmapAvailable || bDIBAvailable || bMFAvailable;
        }

    return bPasteIsAvailable;
    }

/***************************************************************************/

BOOL CImgWnd::IsSelectionAvailable( void )
    {
    if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
        CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();

        if (pTextTool != NULL
        &&  pTextTool->IsKindOf( RUNTIME_CLASS( CTextTool ) ))
            {
            CTedit* pTextEdit = pTextTool->GetTextEditField();

            if (pTextEdit != NULL
            &&  pTextEdit->IsKindOf( RUNTIME_CLASS( CTedit ) ))
                {
                DWORD dwSel = pTextEdit->GetEditWindow()->GetSel();
                BOOL bReturn = (HIWORD( dwSel) != LOWORD( dwSel ));

                if (! bReturn)
                    bReturn = (pTextEdit->GetEditWindow()->GetWindowTextLength()
                           != (int)LOWORD( dwSel ));

                return bReturn;
                }
            }
        }

    if (CImgTool::GetCurrentID() == IDMB_PICKTOOL
    ||  CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        return (theImgBrush.m_pImg != NULL && ! g_bCustomBrush);
        }
    return FALSE;
    }

/***************************************************************************/

int PASCAL CheckPastedSize(int nWidth, int nHeight, IMG* pImg)
{
        int nRet = IDNO;

        // If the image is a bitmap and the bitmap in the clipboard is larger,
        // then give the suer the option2 of growing the image...
        if (nWidth  > pImg->cxWidth
                ||  nHeight > pImg->cyHeight)
        {
                // WARNING!!! MB_SYSTEMMODAL is _necessary_.  No message boxes should
                // be run while the clipboard is opened.  Loss of focus to other apps
                // can be disasterous! The clipboard will be hung or if the clipboard
                // is closed, the contents could be changed by another app.

                nRet = AfxMessageBox( IDS_ENLAGEBITMAPFORCLIP,
                        MB_YESNOCANCEL | MB_ICONQUESTION | MB_SYSTEMMODAL );
                switch (nRet)
                {
                        case IDYES:
                        {
                                CSize size( max(nWidth, pImg->cxWidth),
                                        max(nHeight, pImg->cyHeight) );

                                theUndo.BeginUndo( TEXT("Resize Bitmap") );
                                VERIFY( pImg->m_pBitmapObj->SetSizeProp( P_Size, size ) );

                                theUndo.EndUndo();
                        }
                        break;
                }
        }

        return(nRet);
}


BOOL CImgWnd::PasteImageClip()
    {
    CWaitCursor wait;

    /////////////////////////////////////////////////////////////////////////
    // Find out what format is available on the clipboard. if it is
    //   A. CF_BITMAP only - Set the mask bits opaque and blt the bitmap
    //      into ICimageDC
    // In both cases, if the destination bitmap differs in size from
    // source bitmap, user is asked if he/she wants the src bitmap
    // stretched/clipped to new size
    /////////////////////////////////////////////////////////////////////////
    if (! m_wClipboardFormat)
        m_wClipboardFormat = (WORD)RegisterClipboardFormat( TEXT("MSPaintFreehand") );

    if (m_wClipboardFormat == NULL || ! OpenClipboard())
        {
        TRACE( TEXT("Cannot open clipboard!\n") );
        MessageBeep( 0 );
        return FALSE;
        }

    // Enumerate the cliboard contents to determine what is available.
    // If a CF_BITMAP is seen, set a flag and proceed. If a SDKPAINT
    // private format is seen, stop looking further
    BOOL bBitmapAvailable  = FALSE;
#ifdef FHSELCLIP
    BOOL bPrivateAvailable = FALSE;
#endif // FHSELCLIP
    BOOL bPaletteAvailable = FALSE;
    BOOL bDIBAvailable     = FALSE;
    BOOL bMFAvailable      = FALSE;
    WORD wClipFmt          = 0;

    BITMAP    bmData;
    BOOL      bResizedBitmap = FALSE;
    CPalette* ppalClipboard  = NULL;
    CBitmap*  pbmClipboard   = NULL;
    LPSTR     lpDib          = NULL;
    HPALETTE  hPal           = NULL;
    HBITMAP   hBitmap        = NULL;
    HGLOBAL   hDIB           = NULL;
    HGLOBAL   hMF            = NULL;

        BOOL bGotClip = FALSE;

        hPal = (HPALETTE)GetClipboardData( CF_PALETTE );
        if (hPal)
        {
                bPaletteAvailable = TRUE;
                ppalClipboard = CPalette::FromHandle( hPal );
        }

    if (!bGotClip)
        {
        hDIB = (HGLOBAL)GetClipboardData( CF_DIB );

        if (hDIB)
            {
            lpDib = (LPSTR)::GlobalLock( hDIB );

            if (lpDib)
                {
                bmData.bmWidth  = DIBWidth ( lpDib );
                bmData.bmHeight = DIBHeight( lpDib );

                if (bmData.bmWidth && bmData.bmHeight)
                    {
                    bDIBAvailable = TRUE;
                    bPaletteAvailable = FALSE;
                    }
                }
            }
        #ifdef _DEBUG
        TRACE1( "Loaded the DIB %s.\n", (bDIBAvailable? TEXT("Yes"): TEXT("No")) );
        #endif

        bGotClip = bDIBAvailable;
        }

    if (!bGotClip)
        {
        hBitmap = (HBITMAP)GetClipboardData( CF_BITMAP );

        if (hBitmap)
            {
            pbmClipboard = CBitmap::FromHandle( hBitmap );

            if (pbmClipboard->GetObject( sizeof( BITMAP ), &bmData ))
                {
                bBitmapAvailable = TRUE;

                if (bPaletteAvailable)
                    {
                    if (!ppalClipboard)
                        bBitmapAvailable = FALSE;
                    }
                }
            }

        #ifdef _DEBUG
        TRACE1( "Loaded the Bitmap %s.\n", (bBitmapAvailable? TEXT("Yes"): TEXT("No")) );
        #endif

        bGotClip = bBitmapAvailable;
        }

        if (!bGotClip)
        {
                hMF = (HGLOBAL)GetClipboardData(CF_METAFILEPICT);
                if (hMF)
                {
                        CDC dcMF;

                        if (dcMF.CreateCompatibleDC( NULL ))
                        {
                                if (GetMFDimensions(hMF, dcMF.m_hDC, &bmData.bmWidth,
                                        &bmData.bmHeight, m_pImg))
                                {
                                        bMFAvailable = TRUE;
                                }
                        }
                }

                bGotClip = bMFAvailable;
        }

    if (!bGotClip)
        {
        CloseClipboard();
        return FALSE;
        }

    switch (CheckPastedSize(bmData.bmWidth, bmData.bmHeight, m_pImg))
        {
        default:
            CloseClipboard();
            return TRUE;

        case IDYES:
            bResizedBitmap = TRUE;
            break;

        case IDNO:
            break;
        }

    CDC       stdDC;
    BOOL      bOkay   = FALSE;
    CBitmap*  pbmOld  = NULL;
    CPalette* ppalOld = NULL;

    if (bBitmapAvailable)
        {
        CBitmap   bmClipboard;
        CBitmap*  pbmOldCopy  = NULL;
        CPalette* ppalOldCopy = NULL;
        CDC*      pdcCopy     = NULL;

        if (! stdDC.CreateCompatibleDC( NULL ))
            {
            theApp.SetGdiEmergency();
            goto LReturn;
            }

        pbmOld = stdDC.SelectObject( pbmClipboard );

        if (! pbmOld)
            {
            theApp.SetGdiEmergency();
            goto LReturn;
            }

        if (ppalClipboard)
            {
            ppalOld = stdDC.SelectPalette( ppalClipboard, FALSE );
            stdDC.RealizePalette();
            }

        // duplicate the bitmap
        if (! bmClipboard.CreateBitmap( bmData.bmWidth, bmData.bmHeight,
                                        bmData.bmPlanes, bmData.bmBitsPixel, NULL ))
            {
            theApp.SetMemoryEmergency();
            goto LReturn;
            }

        pdcCopy = new CDC;

        if (pdcCopy == NULL)
            {
            theApp.SetMemoryEmergency();
            goto LReturn;
            }

        if (! pdcCopy->CreateCompatibleDC( NULL ))
            {
            delete pdcCopy;
            theApp.SetGdiEmergency();
            goto LReturn;
            }

        pbmOldCopy = pdcCopy->SelectObject( &bmClipboard );

        if (ppalClipboard)
            {
            ppalOldCopy = pdcCopy->SelectPalette( ppalClipboard, FALSE );
            pdcCopy->RealizePalette();
            }

        pdcCopy->BitBlt( 0, 0, bmData.bmWidth, bmData.bmHeight, &stdDC, 0, 0, SRCCOPY );

        if (ppalOldCopy)
            pdcCopy->SelectPalette( ppalOldCopy, FALSE );

        pdcCopy->SelectObject( pbmOldCopy );
        delete pdcCopy;

        stdDC.SelectObject( &bmClipboard );

        // Unload the bitmap
        stdDC.SelectObject( pbmOld );
        pbmOld = NULL;

        if (ppalOld)
            {
            stdDC.SelectPalette( ppalOld, FALSE );
            ppalOld = NULL;
            }

        stdDC.DeleteDC();
        // Now we convert our nice DDB to a DIB and back so we can
        // convert color bitmaps to monochrome nicely and deal with
        // palette differences...
        DWORD dwSize;

        lpDib = DibFromBitmap( (HBITMAP)bmClipboard.GetSafeHandle(), BI_RGB, 0,
                                                 ppalClipboard, NULL, dwSize );
        }

        if (bMFAvailable)
        {
                CDC dcMF;

                if (dcMF.CreateCompatibleDC(CDC::FromHandle(m_pImg->hDC)))
                {
                        CBitmap bmMF;

                        if (bmMF.CreateCompatibleBitmap(CDC::FromHandle(m_pImg->hDC),
                                bmData.bmWidth, bmData.bmHeight))
                        {
                                dcMF.SelectObject(&bmMF);
                                //not needed for DIBSection!!!
                                if (ppalClipboard)
                                {
                                        dcMF.SelectPalette(ppalClipboard, FALSE);
                                }

                                CRect rc(0, 0, bmData.bmWidth, bmData.bmHeight);

                                PlayMetafileIntoDC(hMF, &rc, dcMF.m_hDC);

                                // Select out the bitmap and palette
                                dcMF.DeleteDC();

                                DWORD dwSize;

                                lpDib = DibFromBitmap((HBITMAP)bmMF.m_hObject, BI_RGB, 0,
                                        ppalClipboard, NULL, dwSize );
                        }
                }
        }

    if (lpDib)
        {
        CPalette* ppalDib = CreateDIBPalette( lpDib );

        ppalDib = FixupDibPalette( lpDib, ppalDib );

        HBITMAP hbmDib = DIBToBitmap( lpDib, theApp.m_pPalette, m_pImg->hDC );

        if (bDIBAvailable)
            ::GlobalUnlock( hDIB );
        else
            FreeDib( lpDib );

        if (hbmDib != NULL
        && stdDC.CreateCompatibleDC( CDC::FromHandle( m_pImg->hDC ) ))
            {
            CRect   rtBrush( 0, 0, bmData.bmWidth, bmData.bmHeight );
            BOOL    bBrushMade = FALSE;
            CBitmap bmDib;

            bmDib.Attach( hbmDib );

            pbmOld = stdDC.SelectObject( &bmDib );

            if (m_pImg->m_pPalette)
                {
                ppalOld = stdDC.SelectPalette( m_pImg->m_pPalette, FALSE );
                stdDC.RealizePalette();
                }

#ifdef FHSELCLIP
            if (bPrivateAvailable)
                {
                HGLOBAL hPts = (HGLOBAL)GetClipboardData( m_wClipboardFormat );

                if (hPts)
                    {
                    short* lpShort = (short*)::GlobalLock( hPts );

                    if (lpShort)
                        {
                        BOOL bError   = FALSE;
                        int  iEntries = *lpShort++;
                        LPPOINT lpPts = (LPPOINT)lpShort;

                        CImgTool::Select( IDMB_PICKRGNTOOL );
                        CFreehandSelectTool* pTool = (CFreehandSelectTool*)CImgTool::GetCurrent();

                        if (pTool)
                            {
                            if (pTool->CreatePolyRegion( GetZoom(), lpPts, iEntries )
                            &&  MakeBrush( stdDC.m_hDC, rtBrush ))
                                {
                                bBrushMade = TRUE;
                                }
                            }
                        ::GlobalUnlock( hPts );
                        }
                    }
                }
#endif // FHSELCLIP

            if (! bBrushMade)
                {
                if (CImgTool::GetCurrentID() != IDMB_PICKTOOL)
                    CImgTool::Select( IDMB_PICKTOOL );

                bBrushMade = MakeBrush( stdDC.m_hDC, rtBrush );
                }

            if (bBrushMade)
                {
                // We have to "move" the brush so it appears...
                CRect rect( 0, 0, theImgBrush.m_rcSelection.Width(),
                                  theImgBrush.m_rcSelection.Height() );

                if (! bResizedBitmap)
                    {
                    // Move the brush so that it is in the upper-left corner of
                    // the view (in case it's scrolled)...
                    rect.OffsetRect( -m_xScroll, -m_yScroll );
                    }
                MoveBrush( rect );

                DirtyImg( m_pImg );

                theImgBrush.m_bFirstDrag = FALSE;

                bOkay = TRUE;
                }
            else
                {
                TRACE( TEXT("Paste: MakeBrush failed!\n") );
                }
            if (ppalOld)
                {
                ppalOld = stdDC.SelectPalette( ppalOld, FALSE );
                ppalOld = NULL;
                }

            stdDC.SelectObject( pbmOld );
            pbmOld = NULL;
            bmDib.Detach();
            }

        if (hbmDib != NULL)
            ::DeleteObject( hbmDib );

        if (ppalDib != NULL)
            delete ppalDib;
        }
LReturn:
    if (pbmOld != NULL)
        stdDC.SelectObject( pbmOld );

    if (ppalOld != NULL)
        stdDC.SelectPalette( ppalOld, FALSE );

    CloseClipboard();

    return bOkay;
    }

/***************************************************************************/
/* very similar to PasteImageClip, but this will paste into an existing    */
/* selection (theImgBrush), resizing it if necessary, and not moving it    */
/***************************************************************************/

BOOL CImgWnd::PasteImageFile( LPSTR lpDib )
    {
    CDC   stdDC;
    CRect cRectSelection = theImgBrush.m_rcSelection;
    BOOL bOkay = FALSE;

    if (lpDib == NULL)
        return bOkay;

    int iWidth  = (int)DIBWidth ( lpDib );
    int iHeight = (int)DIBHeight( lpDib );

    if (CImgTool::GetCurrentID()==IDMB_PICKTOOL && theImgBrush.m_bFirstDrag)
    {
        if (iWidth < theImgBrush.m_size.cx)
            {
            cRectSelection.right = cRectSelection.left + iWidth - 1;
            }
        if (iHeight < theImgBrush.m_size.cy)
            {
            cRectSelection.bottom = cRectSelection.top + iHeight - 1;
            }

        // If the image is a bitmap and the bitmap in the clipboard is larger,
        // then give the user the option of growing the image...
        if (iWidth  > theImgBrush.m_size.cx
        ||  iHeight > theImgBrush.m_size.cy)
            {
            switch (AfxMessageBox( IDS_ENLAGEBITMAPFORCLIP,
                                    MB_YESNOCANCEL | MB_ICONQUESTION ))
                {
                default:
                    return bOkay;
                    break;

                case IDYES:
                    cRectSelection.right  = cRectSelection.left + iWidth  - 1;
                    cRectSelection.bottom = cRectSelection.top  + iHeight - 1;
                    break;

                case IDNO:
                    break;
                }
            }
    }
    else
    {
                int xPos = -m_xScroll;
                int yPos = -m_yScroll;

                switch (CheckPastedSize(iWidth, iHeight, m_pImg))
                {
                case IDYES:
                        xPos = yPos = 0;
                        break;

                case IDNO:
                        break;

                default:
                        return(bOkay);
                }

                CImgTool::Select(IDMB_PICKTOOL);
                cRectSelection = CRect(xPos, yPos, xPos+iWidth, yPos+iHeight);
    }

    MakeBrush( m_pImg->hDC, cRectSelection );
    // MakeBrush sets this
    theImgBrush.m_bFirstDrag = FALSE;

    if (! stdDC.CreateCompatibleDC( CDC::FromHandle( m_pImg->hDC ) ))
        {
        theApp.SetGdiEmergency();
        return bOkay;
        }

    CPalette* ppalDib = CreateDIBPalette( lpDib );

    ppalDib = FixupDibPalette( lpDib, ppalDib );

    HBITMAP hbmDib = DIBToBitmap( lpDib, theApp.m_pPalette, m_pImg->hDC );

    SetUndo( m_pImg );

    if (hbmDib != NULL)
        {
        CBitmap   bmDib;
        CPalette* ppalOld = NULL;
        CBitmap*  pbmOld  = NULL;

        bmDib.Attach( hbmDib );

        pbmOld = stdDC.SelectObject( &bmDib );

        if (m_pImg->m_pPalette)
            {
            ppalOld = stdDC.SelectPalette( m_pImg->m_pPalette, FALSE );
            stdDC.RealizePalette();
            }

        if (MakeBrush( stdDC.m_hDC, CRect( CPoint( 0, 0 ), cRectSelection.Size() ) ))
            {
            theImgBrush.m_bFirstDrag = FALSE;

            // We have to "move" the brush so it appears...
            MoveBrush( cRectSelection );

            DirtyImg( m_pImg );

            bOkay = TRUE;
            }
        else
            {
            TRACE( TEXT("Paste: MakeBrush failed!\n") );
            }

        if (ppalOld != NULL)
            {
            ppalOld = stdDC.SelectPalette( ppalOld, FALSE );
            }

        stdDC.SelectObject( pbmOld );
        bmDib.Detach();

        ::DeleteObject( hbmDib );
        }

    if (ppalDib != NULL)
        delete ppalDib;

    return bOkay;
    }

/***************************************************************************/
// Stolen from PBrush
//
/****************************Module*Header******************************\
* Module Name: metafile.c                                               *
* Routines to paste a metafile as a bitmap.                             *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/

/* Computes dimensions of a metafile picture in pixels */
BOOL GetMFDimensions(
    HANDLE hMF,     /* handle to the CF_METAFILEPICT object from clipbrd */
    HDC hDC,        /* display context */
    long *pWidth,    /* width of picture in pixels, OUT param */
    long *pHeight,   /* height of picture in pixels, OUT param */
    IMG* pImg)
{
    METAFILEPICT FAR *lpMfp, Picture;
    int MapModeOld=0;
    RECT Rect;
    long xScale, yScale, Scale;
    int hRes, vRes;     /* horz and vert resolution, in pixels */
    int hSize, vSize;   /* horz and vert size, in mm */
    int fResult = FALSE;

    if (!hMF || !(lpMfp = (METAFILEPICT FAR *)GlobalLock(hMF)))
        return FALSE;
    /* copy metafile picture hdr */
    Picture = *lpMfp;
    GlobalUnlock(hMF);

    /* Do not modify given DC's attributes */
    SaveDC(hDC);

    /* set the mapping mode */
    MapModeOld = SetMapMode(hDC, Picture.mm);
    if (Picture.mm != MM_ISOTROPIC && Picture.mm != MM_ANISOTROPIC)
    {
        /* For modes other than ISOTROPIC and ANISOTROPIC the picture
         * dimensions are given in logical units.
        /* Convert logical units to pixels. */
        Rect.left = 0; Rect.right = Picture.xExt;
        Rect.top = 0;  Rect.bottom = Picture.yExt;
        if (!LPtoDP(hDC, (LPPOINT)&Rect, 2))
            goto Error;
        *pWidth = Rect.right - Rect.left + 1;
        *pHeight = Rect.bottom - Rect.top + 1;
        fResult = TRUE;
    }
    else    /* ISOTROPIC or ANISOTROPIC mode,
             * using the xExt and yExt, determine pixel width and height of
             * the image */
    {
        hRes = GetDeviceCaps(hDC, HORZRES);
        vRes = GetDeviceCaps(hDC, VERTRES);
        hSize = GetDeviceCaps(hDC, HORZSIZE);
        vSize = GetDeviceCaps(hDC, VERTSIZE);
        if (Picture.xExt == 0)  /* assume default size, aspect ratio */
        {
            *pWidth = pImg->cxWidth;
            *pHeight = pImg->cyHeight;
        }
        else if (Picture.xExt > 0)  /* use suggested size in HIMETRIC units */
        {
            // convert suggested extents(in .01 mm units) for picture to pixel units.

            // xPixelsPermm = hRes/hSize;, yPixelsPermm = vRes/vSize;
            // Use Pixels Per logical unit.
            // *pWidth = Picture.xExt*xPixelsPermm/100;
            // *pHeight = Picture.yExt*yPixelsPermm/100;
            *pWidth = ((long)Picture.xExt * hRes/hSize/100);
            *pHeight = ((long)Picture.yExt * vRes/vSize/100);
        }
        else if (Picture.xExt < 0)  /* use suggested aspect ratio, default size */
        {
            // 1 log unit = .01 mm.
            // (# of log units in imageWid pixels)/xExt;
            xScale = 100L * (long) pImg->cxWidth *
                            hSize/hRes/-Picture.xExt;
            // (# of log units in imageHgt pixels)/yExt;
            yScale = 100L * (long) pImg->cyHeight *
                            vSize/vRes/-Picture.yExt;
            // choose the minimum to accomodate the entire image
            Scale = min(xScale, yScale);
            // use scaled Pixels Per log unit.
            *pWidth = ((long)-Picture.xExt * Scale *
                            hRes/hSize / 100);
            *pHeight = ((long)-Picture.yExt * Scale *
                            vRes/vSize / 100);
        }
        fResult = TRUE;
    }

Error:
    if (MapModeOld)
        SetMapMode(hDC, MapModeOld);    /* select the old mapping mode */
    RestoreDC(hDC, -1);
    return fResult;
}

BOOL PlayMetafileIntoDC(
    HANDLE hMF,
    RECT *pRect,
    HDC hDC)
{
    HBRUSH      hbrBackground;
    METAFILEPICT FAR *lpMfp;

    if (!(lpMfp = (METAFILEPICT FAR *)GlobalLock(hMF)))
        return FALSE;

    SaveDC(hDC);

        /* Setup background color for the bitmap */
        hbrBackground = CreateSolidBrush(crRight);
    FillRect(hDC, pRect, hbrBackground);
    DeleteObject(hbrBackground);

    SetMapMode(hDC, lpMfp->mm);
    if (lpMfp->mm == MM_ISOTROPIC || lpMfp->mm == MM_ANISOTROPIC)
        SetViewportExtEx(hDC, pRect->right-pRect->left, pRect->bottom-pRect->top,
            NULL);
    PlayMetaFile(hDC, lpMfp->hMF);
    GlobalUnlock(hMF);
    RestoreDC(hDC, -1);
    return TRUE;
}

