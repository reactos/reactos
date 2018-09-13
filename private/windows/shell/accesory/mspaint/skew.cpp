/****************************************************************************
 SKEW.c

 The SKEW module handles rotating a rectangular object.

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

static int miAngle;

/***************************************************************************/

static int CalcOffset( int iValue )
    {
    // tan[angle] is the 100000/tangent of angles 1 to 89deg.
    static int invtan[90] = {  -1, 5728996, 2863625, 1908114, 1430067,
                          1143005,  951436,  814436,  711537,  631375,
                           567128,  514455,  470463,  433148,  401078,
                           373205,  348741,  327085,  307768,  290421,
                           274748,  260509,  247509,  235585,  224604,
                           214451,  205030,  196261,  188073,  180405,
                           173205,  166428,  160034,  153987,  148256,
                           142815,  137638,  132705,  127994,  123490,
                           119175,  115037,  111061,  107237,  103553,
                           100000,   96569,   93252,   90040,   86929,
                            83910,   80978,   78129,   75355,   72654,
                            70021,   67451,   64941,   62487,   60086,
                            57735,   55431,   53171,   50953,   48773,
                            46631,   44523,   42448,   40403,   38386,
                            36397,   34433,   32492,   30573,   28675,
                            26795,   24933,   23087,   21256,   19438,
                            17633,   15838,   14054,   12279,   10510,
                             8749,    6993,    5241,    3492,    1746 };
    return (((iValue * 1000000 / invtan[miAngle]) + 5) / 10);
    }

/***************************************************************************/

void CImgWnd::CmdSkewBrush( int iAngle, BOOL bHorz )
    {
    BOOL  bFullImage = FALSE;
    CDC*  pdcImg = CDC::FromHandle( m_pImg->hDC );
    CDC*  pdcSrc = &theImgBrush.m_dc;
    CRect rectSlct( 0, 0, theImgBrush.m_size.cx, theImgBrush.m_size.cy );

    if (theImgBrush.m_pImg == NULL && ! g_bCustomBrush)
        {
        bFullImage = TRUE;
        pdcSrc     = pdcImg;
        rectSlct.SetRect( 0, 0, m_pImg->cxWidth, m_pImg->cyHeight );
        }

    CRect rectSkew = rectSlct;

    if (!iAngle || rectSlct.IsRectEmpty() || abs(iAngle) > 89 || abs(iAngle) < 1 )
        return;

    if (! bFullImage)
        {
        PrepareForBrushChange( TRUE, TRUE );
        HideBrush();
        }
    // angle is the angle of skew 0 to 90 degrees
    // axis is the direction to skew TRUE:  horizontal
    //                                      the y axis moves by angle
    //                               FALSE: verticle
    //                                      the x axis moves by angle
    int iWidthSlct  = rectSlct.Width();
    int iHeightSlct = rectSlct.Height();

    miAngle = abs(iAngle);

    if (bHorz) // horizontal - create a new bitmap of size = x + y * tan (angle)
        rectSkew.right += CalcOffset( iHeightSlct );
    else       // verticale - create a new bitmap of size = y + x * tan (angle)
        rectSkew.bottom += CalcOffset( iWidthSlct );

    int iWidthSkew  = rectSkew.Width();
    int iHeightSkew = rectSkew.Height();

    // angle is the angle of skew 0 to 90 degrees
    // axis is the direction to skew TRUE:  horizontal
    //                                      the y axis moves by angle
    //                               FALSE: verticle
    //                                      the x axis moves by angle
    CDC     dcWork;
    CBitmap bmWork;
    CBrush  brBackground;

    if (! dcWork.CreateCompatibleDC( pdcImg )
    ||  ! bmWork.CreateCompatibleBitmap( pdcImg, iWidthSkew, iHeightSkew )
    ||  ! brBackground.CreateSolidBrush( crRight ))
        {
        theApp.SetGdiEmergency( TRUE );
        return;
        }

    CBitmap*  pbmOld = dcWork.SelectObject( &bmWork );
    CPalette* ppalOld = SetImgPalette( &dcWork, FALSE );

    dcWork.FillRect( rectSkew, &brBackground );

    CPalette* ppalOldSrc = SetImgPalette( pdcSrc, FALSE );

    // Skew the bitmap
    int mx = 1;
    int my = 1;  // local variables for the skew
    int x = rectSlct.left;
    int y = rectSlct.top;

    BeginWaitCursor();

    if (bHorz) // horizontal
        {
        if (iAngle > 0)
            {
            mx = x + CalcOffset( iHeightSlct - y );
            // Start at Bottom
            for (y = rectSlct.bottom; y >= rectSlct.top; )
                {
                dcWork.BitBlt( mx, y, iWidthSlct, 1, pdcSrc, x, y, SRCCOPY );
                mx = x + CalcOffset( iHeightSlct - y-- );
                }
            }
        else
            {
            x = rectSkew.right - iWidthSlct;
            mx = x - CalcOffset( iHeightSlct - y );
            for (y = rectSlct.bottom; y >= rectSlct.top; )
                {
                dcWork.BitBlt( mx, y, iWidthSlct, 1, pdcSrc, rectSlct.left, y, SRCCOPY );
                mx = x - CalcOffset( iHeightSlct - y-- );
                }
            }
        }
    else
        { // vertical
        if (iAngle > 0)
            {
            my = y + CalcOffset( iWidthSlct - x );
            // Start at left
            for (x = rectSlct.left - 1; x <= rectSlct.right; )
                {
                dcWork.BitBlt( x, my, 1, iHeightSlct, pdcSrc, x, y, SRCCOPY );
                my = y + CalcOffset( iWidthSlct - x++ );
                }
            }
        else
            {
            y = rectSkew.bottom - iHeightSlct;
            my = y - CalcOffset( iWidthSlct - x );
            for (x = rectSlct.left - 1; x <= rectSlct.right; )
                {
                dcWork.BitBlt( x, my, 1, iHeightSlct, pdcSrc, x, rectSlct.top, SRCCOPY );
                my = y - CalcOffset( iWidthSlct - x++ );
                }
            }
        }

    if (ppalOldSrc)
        pdcSrc->SelectPalette( ppalOldSrc, FALSE );

    if (bFullImage)
        {
        theUndo.BeginUndo( TEXT("Skew Bitmap") );

        m_pImg->m_pBitmapObj->SetSizeProp( P_Size, CSize( iWidthSkew, iHeightSkew ) );

        m_pImg->cxWidth  = iWidthSkew;
        m_pImg->cyHeight = iHeightSkew;

        SetUndo( m_pImg );

        pdcImg->BitBlt( 0, 0, m_pImg->cxWidth, m_pImg->cyHeight, &dcWork, 0, 0, SRCCOPY );

        InvalImgRect ( m_pImg, &rectSkew );
        CommitImgRect( m_pImg, &rectSkew );

        FinishUndo( rectSkew );

        theUndo.EndUndo();

        DirtyImg( m_pImg );

        InvalidateRect( NULL );
        UpdateWindow();

        dcWork.SelectObject( pbmOld );
        bmWork.DeleteObject();
        }
    else
        {
        CBitmap bmMask;

        if (! bmMask.CreateBitmap( iWidthSkew, iHeightSkew, 1, 1, NULL ))
            {
            if (ppalOld)
                dcWork.SelectPalette( ppalOld, FALSE );

            theApp.SetMemoryEmergency( TRUE );
            return;
            }

        dcWork.SelectObject( pbmOld );

        theImgBrush.m_dc.SelectObject( &bmWork );
        theImgBrush.m_bitmap.DeleteObject();
        theImgBrush.m_bitmap.Attach( bmWork.Detach() );

        theImgBrush.m_size.cx = iWidthSkew;
        theImgBrush.m_size.cy = iHeightSkew;

        VERIFY( theImgBrush.m_maskDC.SelectObject( &bmMask ) ==
               &theImgBrush.m_maskBitmap );

        theImgBrush.m_maskBitmap.DeleteObject();
        theImgBrush.m_maskBitmap.Attach( bmMask.Detach() );
        theImgBrush.RecalcMask( crRight );

        rectSkew = theImgBrush.m_rcSelection;

        mx = 0;
        my = 0;

        if (bHorz)
            mx = CalcOffset( iHeightSlct ) / 2;
        else
            my = CalcOffset( iWidthSlct ) / 2;

        rectSkew.InflateRect( mx, my );

        MoveBrush( rectSkew );
        }

    if (ppalOld)
        dcWork.SelectPalette( ppalOld, FALSE );

    dcWork.DeleteDC();

    EndWaitCursor();
    }

/***************************************************************************/

