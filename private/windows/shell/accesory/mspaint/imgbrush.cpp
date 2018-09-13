
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
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "t_fhsel.h"
#include "toolbox.h"

#include "mmsystem.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

CImgBrush theImgBrush;

CImgBrush::CImgBrush() : m_bCuttingFromImage(FALSE), m_bOpaque(TRUE)
    {
    }

CImgBrush::~CImgBrush()
    {
    // cleanup old region if exists
    if (m_cRgnPolyFreeHandSel.GetSafeHandle() != NULL)
        m_cRgnPolyFreeHandSel.DeleteObject();

    if (m_cRgnPolyFreeHandSelBorder.GetSafeHandle() != NULL)
        m_cRgnPolyFreeHandSelBorder.DeleteObject();

    if (m_hbmOld)
        m_dc.SelectObject( CBitmap::FromHandle( m_hbmOld ) );

    if (m_hbmMaskOld)
        m_dc.SelectObject( CBitmap::FromHandle( m_hbmMaskOld ) );

    m_dc.DeleteDC();
    m_bitmap.DeleteObject();
    m_maskDC.DeleteDC();
    m_maskBitmap.DeleteObject();
    }

BOOL CImgBrush::CopyTo(CImgBrush& destBrush)
    {
    if (destBrush.m_hbmOld)
        destBrush.m_dc.SelectObject( CBitmap::FromHandle( destBrush.m_hbmOld ) );

    if (destBrush.m_hbmMaskOld)
        destBrush.m_maskDC.SelectObject( CBitmap::FromHandle( destBrush.m_hbmMaskOld ) );

    destBrush.m_hbmOld     = NULL;
    destBrush.m_hbmMaskOld = NULL;

    destBrush.m_dc.DeleteDC();
    destBrush.m_bitmap.DeleteObject();
    destBrush.m_maskDC.DeleteDC();
    destBrush.m_maskBitmap.DeleteObject();

    if (m_dc.m_hDC != NULL)
        {
        if (! destBrush.m_dc.CreateCompatibleDC( NULL )
        ||  ! destBrush.m_bitmap.CreateCompatibleBitmap(&m_dc, m_size.cx, m_size.cy))
            {
            theApp.SetGdiEmergency(FALSE);
            return FALSE;
            }

        destBrush.m_hbmOld = (HBITMAP)((destBrush.m_dc.SelectObject( &destBrush.m_bitmap ))->GetSafeHandle());

        CPalette* ppalOldSrc  = SetBrushPalette(           &m_dc, TRUE ); // Background ??
        CPalette* ppalOldDest = SetBrushPalette( &destBrush.m_dc, TRUE ); // Background ??

        destBrush.m_dc.BitBlt( 0, 0, m_size.cx, m_size.cy, &m_dc, 0, 0, SRCCOPY );

        if (ppalOldSrc)
            m_dc.SelectPalette( ppalOldSrc, TRUE ); // Background ??

        if (ppalOldDest)
            destBrush.m_dc.SelectPalette( ppalOldDest, TRUE ); // Background ??
        }

    if (m_maskDC.m_hDC != NULL)
        {
        if (! destBrush.m_maskDC.CreateCompatibleDC(NULL)
        ||  ! destBrush.m_maskBitmap.CreateCompatibleBitmap(&m_maskDC, m_size.cx,
                                                                       m_size.cy))
            {
            theApp.SetGdiEmergency(FALSE);
            return FALSE;
            }

        destBrush.m_hbmMaskOld = (HBITMAP)((destBrush.m_maskDC.SelectObject(
                                           &destBrush.m_maskBitmap))->GetSafeHandle());
        destBrush.m_maskDC.BitBlt(0, 0, m_size.cx, m_size.cy, &m_maskDC, 0, 0, SRCCOPY);
        }

    destBrush.m_size               = m_size;
    destBrush.m_bFirstDrag         = m_bFirstDrag;
    destBrush.m_bLastDragWasASmear = m_bLastDragWasASmear;
    destBrush.m_bLastDragWasFirst  = m_bLastDragWasFirst;
    destBrush.m_bMakingSelection   = m_bMakingSelection;
    destBrush.m_bMoveSel           = m_bMoveSel;
    destBrush.m_bSmearSel          = m_bSmearSel;
    destBrush.m_bOpaque            = m_bOpaque;
    destBrush.m_rcDraggedFrom      = m_rcDraggedFrom;
    destBrush.m_pImg               = m_pImg;
    destBrush.m_rcSelection        = m_rcSelection;
    destBrush.m_handle             = m_handle;

    return TRUE;
    }


void CImgBrush::CenterHandle()
    {
    m_handle.cx = m_size.cx / 2;
    m_handle.cy = m_size.cy / 2;
    }


void CImgBrush::TopLeftHandle()
    {
    m_handle.cx = 0;
    m_handle.cy = 0;
    }

CPalette* CImgBrush::SetBrushPalette( CDC* pdc, BOOL bForce )
    {
    CPalette* ppal    = NULL;
    CPalette* ppalOld = NULL;

    if (theApp.m_pPalette
    &&  theApp.m_pPalette->m_hObject)
        ppal = theApp.m_pPalette;

    if (ppal != NULL)
        {
        ppalOld = pdc->SelectPalette( ppal, bForce );

        pdc->RealizePalette();
        }
    return ppalOld;
    }

HPALETTE CImgBrush::SetBrushPalette( HDC hdc, BOOL bForce )
    {
    CPalette* ppal    = NULL;
    HPALETTE  hpalOld = NULL;

    if (theApp.m_pPalette
    &&  theApp.m_pPalette->m_hObject)
        ppal = theApp.m_pPalette;

    if (ppal != NULL)
        {
        hpalOld = ::SelectPalette( hdc, (HPALETTE)ppal->m_hObject, bForce );

        RealizePalette( hdc );
        }
    return hpalOld;
    }

BOOL CImgBrush::SetSize( CSize newSize, BOOL bStretchToFit )
    {
    BOOL bFlipX;
    BOOL bFlipY;

    if (newSize.cx == m_size.cx
    &&  newSize.cy == m_size.cy)
        return TRUE;

    if (bFlipX = (newSize.cx < 0))
        newSize.cx = -newSize.cx;

    if (bFlipY = (newSize.cy < 0))
        newSize.cy = -newSize.cy;

    if (newSize.cx == 0)
        newSize.cx  = 1;

    if (newSize.cy == 0)
        newSize.cy  = 1;

    if (CImgTool::GetCurrentID() != IDMX_TEXTTOOL)
        {
        CDC     newDC;
        CBitmap newBitmap;
        CDC     newMaskDC;
        CBitmap newMaskBitmap;

                //BUGBUG-Potential for DC && Bitmap leaks here on partial failure!!
        if (!         newDC.CreateCompatibleDC    ( &m_dc )
        ||  !     newBitmap.CreateCompatibleBitmap( &m_dc, newSize.cx, newSize.cy )
        ||  !     newMaskDC.CreateCompatibleDC    ( &m_dc )
        ||  ! newMaskBitmap.CreateBitmap(newSize.cx, newSize.cy, 1, 1, NULL ))
            return FALSE;

        CBitmap*  pbmOld      = newDC.SelectObject( &newBitmap );
        CPalette* ppalOldSrc  = SetBrushPalette(  &m_dc, FALSE );
        CPalette* ppalOldDest = SetBrushPalette( &newDC, FALSE );

        newDC.SetStretchBltMode(COLORONCOLOR);


        if (bStretchToFit)
            {
            StretchCopy( newDC.m_hDC, bFlipX ?  newSize.cx : 0,
                                      bFlipY ?  newSize.cy : 0,
                                      bFlipX ? -newSize.cx : newSize.cx,
                                      bFlipY ? -newSize.cy : newSize.cy,
                          m_dc.m_hDC, 0, 0, m_size.cx, m_size.cy );
            }
        else
            {
            StretchCopy( newDC.m_hDC, bFlipX ? newSize.cx : 0,
                                      bFlipY ? newSize.cy : 0,
                                      m_size.cx, m_size.cy,
                    m_dc.m_hDC, 0, 0, m_size.cx, m_size.cy );
            }


        COLORREF crOldBk    = newDC.SetBkColor( crRight );
        CBitmap* pbmOldMask = newMaskDC.SelectObject( &newMaskBitmap );

        if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
            {
            CFreehandSelectTool* pTool = (CFreehandSelectTool*)CImgTool::GetCurrent();

            if (pTool->ExpandPolyRegion( newSize.cx, newSize.cy ))
                {
                newMaskDC.PatBlt( 0, 0, newSize.cx, newSize.cy, BLACKNESS );

                if (m_cRgnPolyFreeHandSel.GetSafeHandle())
                    newMaskDC.FillRgn( &m_cRgnPolyFreeHandSel,
                                        CBrush::FromHandle( (HBRUSH)::GetStockObject( WHITE_BRUSH ) ) );
                }
            }
        else
            {
            newMaskDC.PatBlt( 0, 0, newSize.cx, newSize.cy, WHITENESS );
            }

        newMaskDC.BitBlt( 0, 0, newSize.cx, newSize.cy, &newDC, 0, 0, DSna );
        newDC.SetBkColor( crOldBk );

        if (ppalOldSrc)
            m_dc.SelectPalette( ppalOldSrc, FALSE );

        if (ppalOldDest)
            newDC.SelectPalette( ppalOldDest, FALSE );

        if (m_hbmOld)
            m_dc.SelectObject( CBitmap::FromHandle( m_hbmOld ) );

        if (m_hbmMaskOld)
            m_maskDC.SelectObject( CBitmap::FromHandle( m_hbmMaskOld ) );

                m_dc.DeleteDC();
            m_maskDC.DeleteDC();
            m_bitmap.DeleteObject();
        m_maskBitmap.DeleteObject();

                m_dc.Attach(         newDC.Detach() );
            m_bitmap.Attach(     newBitmap.Detach() );
            m_maskDC.Attach(     newMaskDC.Detach() );
        m_maskBitmap.Attach( newMaskBitmap.Detach() );

        m_hbmOld     = (HBITMAP)(pbmOld->GetSafeHandle());
        m_hbmMaskOld = (HBITMAP)(pbmOldMask->GetSafeHandle());
        }

    m_size.cx = newSize.cx;
    m_size.cy = newSize.cy;

    return TRUE;
    }


#if defined(DEBUGSHOWBITMAPS)
void DebugShowBitmap(HDC hdcSrc, int x, int y, int wid, int hgt)
{
        HDC hdcDst = GetDC(NULL);
        BitBlt(hdcDst, 0, 0, wid, hgt, hdcSrc, x, y, SRCCOPY);
        ReleaseDC(NULL, hdcDst);
}
#endif


BOOL QuickColorToMono(CDC* pdcMono, int xMono, int yMono, int cx, int cy,
        CDC *pdcColor, int xColor, int yColor, DWORD dwROP, COLORREF crTrans)
{
        RGBQUAD rgb[256];
        int nColors = GetDIBColorTable(pdcColor->m_hDC, 0, 256, &rgb[0]);
        if (nColors == 0)
        {
                return(FALSE);
        }

        RGBQUAD rgbWhite;
        rgbWhite.rgbRed   = 255;
        rgbWhite.rgbGreen = 255;
        rgbWhite.rgbBlue  = 255;
        rgbWhite.rgbReserved = 0;

        RGBQUAD rgbBlack;
        rgbBlack.rgbRed   = 0;
        rgbBlack.rgbGreen = 0;
        rgbBlack.rgbBlue  = 0;
        rgbBlack.rgbReserved = 0;

        RGBQUAD rgbTemp[256];

        switch ((BYTE)((crTrans)>>24))
        {
        case 0:
        case 2:
        {
                RGBQUAD rgbTrans;
                rgbTrans.rgbRed   = GetRValue(crTrans);
                rgbTrans.rgbGreen = GetGValue(crTrans);
                rgbTrans.rgbBlue  = GetBValue(crTrans);
                rgbTrans.rgbReserved = 0;

                for (int nColor=nColors-1; nColor>=0; --nColor)
                {
                        if (memcmp(&rgb[nColor], &rgbTrans, sizeof(rgbTrans)) == 0)
                        {
                                rgbTemp[nColor] = rgbWhite;
                        }
                        else
                        {
                                rgbTemp[nColor] = rgbBlack;
                        }
                }

                break;
        }

        // We can put support for different COLORREF formats here

        default:
                return(FALSE);
        }

        SetDIBColorTable(pdcColor->m_hDC, 0, nColors, &rgbTemp[0]);
        pdcMono->BitBlt(xMono, yMono, cx, cy, pdcColor, xColor, yColor, dwROP);
        SetDIBColorTable(pdcColor->m_hDC, 0, nColors, &rgb[0]);

        return(TRUE);
}


#define COLORTOMONOBUG
#ifdef  COLORTOMONOBUG
void CImgBrush::ColorToMonoBitBlt(CDC* pdcMono, int xMono, int yMono, int cx, int cy,
        CDC *pdcColor, int xColor, int yColor, DWORD dwROP, COLORREF transparentColor)
{
        if (QuickColorToMono(pdcMono, xMono, yMono, cx, cy,
                pdcColor, xColor, yColor, dwROP, transparentColor))
        {
                return;
        }

        CDC dcColor;
        CTempBitmap bmColor;
        CBitmap* pbmOldColor = NULL;

        // Use a moderate-sized intermediate bitmap
        int cyStep = 0xffff / cx;
        cyStep = max(1, min(cy, cyStep));

        HDC hdcScreen = GetDC(NULL);
        BOOL bError = (!dcColor.CreateCompatibleDC(NULL)
                        || !bmColor.CreateCompatibleBitmap(CDC::FromHandle(hdcScreen), cx, cyStep)
                        || (pbmOldColor = dcColor.SelectObject(&bmColor))==NULL);
        ReleaseDC(NULL, hdcScreen);

        if (bError)
        {
                theApp.SetGdiEmergency(FALSE);
                return;
        }

        CPalette* ppalOld  = SetBrushPalette( pdcColor, FALSE );
        CPalette* ppalOldColor  = SetBrushPalette( &dcColor, FALSE );
        dcColor.SetBkColor( transparentColor );

        int yStep;
        for (yStep=0; yStep<cy; yStep+=cyStep)
        {
                if (cyStep > cy - yStep)
                {
                        cyStep = cy - yStep;
                }

                dcColor.BitBlt(0, 0, cx, cyStep, pdcColor, xColor, yColor+yStep, SRCCOPY);
                pdcMono->BitBlt(xMono, yMono+yStep, cx, cyStep, &dcColor, 0, 0, dwROP);

                DebugShowBitmap(pdcMono->m_hDC, xMono, yMono, cx, cy);
        }

        if (ppalOld)
        {
                pdcColor->SelectPalette( ppalOld, FALSE );
        }

        dcColor.SelectObject(pbmOldColor);

        if (ppalOldColor)
        {
                dcColor.SelectPalette( ppalOldColor, FALSE );
        }
}
#endif  // COLORTOMONOBUG

void CImgBrush::RecalcMask( COLORREF transparentColor )
    {
    if (! m_dc.GetSafeHdc() || ! m_maskDC.m_hDC)
        return;

#ifndef COLORTOMONOBUG
    CPalette* ppalOld  = SetBrushPalette( &m_dc, FALSE );
    COLORREF oldBkColor = m_dc.SetBkColor( transparentColor );
#endif  // COLORTOMONOBUG

    if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        m_maskDC.PatBlt( 0, 0, m_size.cx, m_size.cy, BLACKNESS );

        if (m_cRgnPolyFreeHandSel.GetSafeHandle())
            m_maskDC.FillRgn( &m_cRgnPolyFreeHandSel,
                            CBrush::FromHandle( (HBRUSH)::GetStockObject( WHITE_BRUSH ) ) );

        if (! m_bOpaque) // can't test true, since bitfield
#ifdef  COLORTOMONOBUG
            ColorToMonoBitBlt(&m_maskDC, 0, 0, m_size.cx, m_size.cy,
                &m_dc, 0, 0, DSna, transparentColor);
#else   // COLORTOMONOBUG
            m_maskDC.BitBlt( 0, 0, m_size.cx, m_size.cy, &m_dc, 0, 0, DSna );
#endif  // COLORTOMONOBUG
        }
    else
        {
#ifdef  COLORTOMONOBUG
        ColorToMonoBitBlt(&m_maskDC, 0, 0, m_size.cx, m_size.cy,
            &m_dc, 0, 0, NOTSRCCOPY, transparentColor);
#else   // COLORTOMONOBUG
        m_maskDC.BitBlt( 0, 0, m_size.cx, m_size.cy, &m_dc, 0, 0, NOTSRCCOPY );
#endif  // COLORTOMONOBUG
        }

#ifndef COLORTOMONOBUG
    m_dc.SetBkColor( oldBkColor );

    if (ppalOld)
        m_dc.SelectPalette( ppalOld, FALSE );
#endif  // COLORTOMONOBUG
    }


void GetMonoBltColors(HDC hDC, HBITMAP hBM, COLORREF& crNewBk, COLORREF& crNewText)
{
        crNewBk   = RGB(0xff, 0xff, 0xff);
        crNewText = RGB(0x00, 0x00, 0x00);

        RGBQUAD rq;
        if (GetDIBColorTable(hDC, 0, 1, &rq) == 1)
        {
                crNewBk   = DIBINDEX(0xff);
                crNewText = DIBINDEX(0x00);
        }
}


// This handles drawing the brush with Draw Opaque turned off.
//
void CImgBrush::BltMatte(IMG* pimg, CPoint topLeft)
    {
        COLORREF crNewBk, crNewText;
        GetMonoBltColors(pimg->hDC, pimg->hBitmap, crNewBk, crNewText);

    COLORREF crOldBk = SetBkColor(pimg->hDC, crNewBk);
    COLORREF crOldText = SetTextColor(pimg->hDC, crNewText);
    CPalette* ppalOld = SetBrushPalette( &m_dc, FALSE ); // Background ??

    // Copying from a bitmap...

DebugShowBitmap(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy);
    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
          m_dc.m_hDC, 0, 0, DSx);
DebugShowBitmap(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy);

    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
      m_maskDC.m_hDC, 0, 0, DSna);
DebugShowBitmap(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy);

    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
          m_dc.m_hDC, 0, 0, DSx);
DebugShowBitmap(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy);

    if (ppalOld)
        m_dc.SelectPalette( ppalOld, FALSE ); // Background ??

    SetBkColor(pimg->hDC, crOldBk);
    SetTextColor(pimg->hDC, crOldText);
    }

// This handles drawing the brush with Draw Opaque turned on.
//
void CImgBrush::BltReplace(IMG* pimg, CPoint topLeft)
    {
    int iToolID =  CImgTool::GetCurrentID();

    if (iToolID == IDMB_PICKRGNTOOL)
        {
        BltMatte( pimg, topLeft );
        }
    else
        {
        CPalette* ppalOld = SetBrushPalette( &m_dc, FALSE ); // Background ??

        BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
               m_dc.m_hDC, 0, 0, SRCCOPY);

        if (ppalOld)
            m_dc.SelectPalette( ppalOld, FALSE ); // Background ??
        }
    }


// This handles erasing with the brush (draws mask in solid color).
//
void CImgBrush::BltColor(IMG* pimg, CPoint topLeft, COLORREF color)
    {
    COLORREF crOldBk = SetBkColor(pimg->hDC, color);
    COLORREF crOldText = SetTextColor(pimg->hDC, RGB(0, 0, 0));

    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
      m_maskDC.m_hDC, 0, 0, DSx);

    SetBkColor(pimg->hDC, RGB(255, 255, 255));

    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
      m_maskDC.m_hDC, 0, 0, DSna);

    SetBkColor(pimg->hDC, color);

    BitBlt(pimg->hDC, topLeft.x, topLeft.y, m_size.cx, m_size.cy,
      m_maskDC.m_hDC, 0, 0, DSx);

    SetTextColor(pimg->hDC, crOldText);
    SetBkColor(pimg->hDC, crOldBk);
    }


