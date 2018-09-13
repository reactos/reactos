/****************************************************************************
 ROTATE.c

 The ROTATE module handles rotating a rectangular object.

****************************************************************************/

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgsuprt.h"
#include "bmobject.h"
#include "undo.h"
#include "props.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

/***************************************************************************/

void CImgWnd::CmdRot90()
    {
    HideBrush();

    CRect     rotRect;
    HDC       hdcSrc;
    CPalette* ppalOld = NULL;

    if (! theImgBrush.m_pImg && ! g_bCustomBrush)
        {
        rotRect.SetRect( 0, 0, m_pImg->cxWidth, m_pImg->cyHeight );

        hdcSrc = m_pImg->hDC;
        }
    else
        {
        PrepareForBrushChange( TRUE, TRUE );

        ppalOld = SetImgPalette( &theImgBrush.m_dc );

        hdcSrc = theImgBrush.m_dc.GetSafeHdc();

        rotRect         = rcDragBrush;
        rotRect.right  -= 1;
        rotRect.bottom -= 1;
        }

    ASSERT( hdcSrc != NULL);

    if (rotRect.IsRectEmpty() || ! hdcSrc)
        {
        if (ppalOld)
            theImgBrush.m_dc.SelectPalette( ppalOld, FALSE );

        return;
        }

    int iWidth  = rotRect.Width();
    int iHeight = rotRect.Height();

    CRect destRect( 0, 0, iHeight, iWidth );

    destRect.OffsetRect( rotRect.left + iWidth  / 2 - iHeight / 2,
                         rotRect.top  + iHeight / 2 - iWidth  / 2 );
    CBitmap bmRotated;
    CDC     dcRotated;
    CDC*    pdcImg = CDC::FromHandle( m_pImg->hDC );

    if (! bmRotated.CreateCompatibleBitmap( pdcImg, iHeight, iWidth )
    ||  ! dcRotated.CreateCompatibleDC    ( pdcImg ))
        {
        if (ppalOld)
            theImgBrush.m_dc.SelectPalette( ppalOld, FALSE );

        theApp.SetGdiEmergency( TRUE );
        return;
        }

    CBitmap*  pbmOld = dcRotated.SelectObject( &bmRotated );
    CPalette* ppalRotated = SetImgPalette( &dcRotated );

    BeginWaitCursor();

    int  iRow;
    int  iCol;
    BOOL bDone = FALSE;

    // Need code here to get select RECT from the hdcSrc
    HDC     rowDC  = ::CreateCompatibleDC    ( hdcSrc );
    HDC     colDC  = ::CreateCompatibleDC    ( hdcSrc );
    HBITMAP hrowBM = ::CreateCompatibleBitmap( hdcSrc, iWidth, 1 );
    HBITMAP hcolBM = ::CreateCompatibleBitmap( hdcSrc, 1, iWidth );

    if (rowDC && colDC && hrowBM && hcolBM)
        {
        HBITMAP scolBM  = (HBITMAP)::SelectObject( colDC, hcolBM );
        HBITMAP srowBM  = (HBITMAP)::SelectObject( rowDC, hrowBM );

        ::PatBlt( rowDC, 0, 0, iWidth, 1, BLACKNESS );
        ::PatBlt( colDC, 0, 0, 1, iWidth, BLACKNESS );

        HPALETTE hpalRow = SetImgPalette( rowDC ); // save to replace later
        HPALETTE hpalCol = SetImgPalette( colDC ); // save to replace later

        ::SelectObject( colDC, scolBM );
        ::SelectObject( rowDC, srowBM );

        DWORD dwLen;

        LPSTR lpDibRow = DibFromBitmap( hrowBM, DIB_RGB_COLORS, 24,
                                        theApp.m_pPalette, NULL, dwLen );

        LPSTR lpDibCol = DibFromBitmap( hcolBM, DIB_RGB_COLORS, 24,
                                        theApp.m_pPalette, NULL, dwLen );
        if (lpDibRow && lpDibCol)
            {
            VOID* pBitsRow = FindDIBBits( lpDibRow );
            VOID* pBitsCol = FindDIBBits( lpDibCol );

            for (iRow = 0, iCol = iHeight - 1; iRow < iHeight; iRow++, iCol--)
                {
                ::SelectObject( rowDC, hrowBM );
                ::BitBlt( rowDC, 0, 0, iWidth, 1, hdcSrc, 0, iRow, SRCCOPY );
                ::SelectObject( rowDC, srowBM );

                if (! GetDIBits( hdcSrc, hrowBM, 0, 1, pBitsRow, (LPBITMAPINFO)lpDibRow, DIB_RGB_COLORS ))
                    break;

                LPBYTE  pRow =  (LPBYTE)pBitsRow;
                LPDWORD pCol = (LPDWORD)pBitsCol;

                union
                    {
                    DWORD pixel;
                    char  byte[sizeof( DWORD )];
                    } u;

                u.byte[3] = 0;

                for (register int index = iWidth - 1; index >= 0; index--)
                    {
                    u.byte[0] = *pRow++;
                    u.byte[1] = *pRow++;
                    u.byte[2] = *pRow++;

                    pCol[index] = u.pixel;
                    }

                if (! SetDIBits( hdcSrc, hcolBM, 0, iWidth, pBitsCol, (LPBITMAPINFO)lpDibCol, DIB_RGB_COLORS ))
                    break;

                ::SelectObject( colDC, hcolBM );
                ::BitBlt( dcRotated.m_hDC, iCol, 0, 1, iWidth, colDC, 0, 0, SRCCOPY );
                ::SelectObject( colDC, scolBM );
                }

            bDone = (iRow == iHeight);

            if (! bDone)
                theApp.SetGdiEmergency( TRUE );
            }
        else
            theApp.SetMemoryEmergency( TRUE );

        if (lpDibRow)
            FreeDib( lpDibRow );

        if (lpDibCol)
            FreeDib( lpDibCol );

        if (hpalRow)
            ::SelectPalette( rowDC, hpalRow, FALSE );

        if (hpalCol)
            ::SelectPalette( colDC, hpalCol, FALSE );
        }
    else
        theApp.SetGdiEmergency( TRUE );

    // clean up
    if (rowDC)
        ::DeleteDC( rowDC );

    if (colDC)
        ::DeleteDC( colDC );

    if (hrowBM)
        ::DeleteObject( hrowBM );

    if (hcolBM)
        ::DeleteObject( hcolBM );

    EndWaitCursor();

    if (! bDone) // do the brute force method
        {
        if (ppalOld)
            theImgBrush.m_dc.SelectPalette( ppalOld, FALSE );

        if (ppalRotated)
            dcRotated.SelectPalette( ppalRotated, FALSE );

        dcRotated.SelectObject( pbmOld );
        dcRotated.DeleteDC();
        bmRotated.DeleteObject();
        return;
        }

    if (ppalOld)
        theImgBrush.m_dc.SelectPalette( ppalOld, FALSE );

    if (  theImgBrush.m_pImg
    &&  ! theImgBrush.m_bFirstDrag || g_bCustomBrush)
        {
        if (ppalRotated)
            dcRotated.SelectPalette( ppalRotated, FALSE );

        dcRotated.SelectObject( pbmOld );

        CBitmap bmMask;

        if (! bmMask.CreateBitmap( iHeight, iWidth, 1, 1, NULL ))
            {
            theApp.SetMemoryEmergency( TRUE );
            return;
            }

        theImgBrush.m_dc.SelectObject( &bmRotated );
        theImgBrush.m_bitmap.DeleteObject();
        theImgBrush.m_bitmap.Attach( bmRotated.Detach() );

        theImgBrush.m_size.cx = iHeight;
        theImgBrush.m_size.cy = iWidth;

        VERIFY( theImgBrush.m_maskDC.SelectObject( &bmMask ) ==
               &theImgBrush.m_maskBitmap );

        theImgBrush.m_maskBitmap.DeleteObject();
        theImgBrush.m_maskBitmap.Attach( bmMask.Detach() );
        theImgBrush.RecalcMask( crRight );

        MoveBrush( destRect );
        }
    else
        {
        theUndo.BeginUndo( TEXT("Resize Bitmap") );

        m_pImg->m_pBitmapObj->SetSizeProp( P_Size, CSize( iHeight, iWidth ) );

        m_pImg->cxWidth  = iHeight;
        m_pImg->cyHeight = iWidth;

        SetUndo( m_pImg );

        pdcImg->BitBlt( 0, 0, m_pImg->cxWidth, m_pImg->cyHeight, &dcRotated, 0, 0, SRCCOPY );

        dcRotated.SelectObject( pbmOld );

        if (ppalRotated)
            dcRotated.SelectPalette( ppalRotated, FALSE );

        bmRotated.DeleteObject();

        InvalImgRect ( m_pImg, &rotRect );
        CommitImgRect( m_pImg, &rotRect );

        FinishUndo   ( rotRect );

        theUndo.EndUndo();

        DirtyImg     ( m_pImg );

        InvalidateRect( NULL );
        UpdateWindow();
        }

    dcRotated.DeleteDC();
    }

/***************************************************************************/
