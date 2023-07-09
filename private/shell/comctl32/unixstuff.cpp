
#include "ctlspriv.h"
#include "unixstuff.h"

EXTERN_C void UnixPaintArrow(HDC hDC, BOOL bHoriz, BOOL bDown, int nXCenter, int nYCenter,
          int nWidth, int nHeight)
{
    HPEN hOldPen;
    HBRUSH hOldBrush;
    POINT pFigure[4];
    LOGBRUSH  hBrLog  = { BS_SOLID, GetSysColor( COLOR_BTNTEXT ), 0   };

    HBRUSH hBrush = CreateBrushIndirect( &hBrLog  );
    HPEN   hPen   = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNTEXT ) );

    if( hPen && hBrush )
    {
        hOldPen           = (HPEN)   SelectObject(hDC, hPen);
        hOldBrush         = (HBRUSH) SelectObject(hDC, hBrush);
    }
    else
    {
        hOldPen   = (HPEN)   SelectObject(hDC, GetStockObject( BLACK_PEN ) );
        hOldBrush = (HBRUSH) SelectObject(hDC, GetStockObject( BLACK_BRUSH ) );
    }

    if(bHoriz) {
       if (bDown) {
                pFigure[0].x = nXCenter - nWidth /2;
                pFigure[0].y = nYCenter - nHeight/2;
                pFigure[1].x = nXCenter - nWidth /2;
                pFigure[1].y = nYCenter + nHeight/2;
                pFigure[2].x = nXCenter + nWidth /2;
                pFigure[2].y = nYCenter;
        } else {
                pFigure[0].x = nXCenter - nWidth /2;
                pFigure[0].y = nYCenter;
                pFigure[1].x = nXCenter + nWidth /2;
                pFigure[1].y = nYCenter + nHeight/2;
                pFigure[2].x = nXCenter + nWidth /2;
                pFigure[2].y = nYCenter - nHeight/2;
        }
    }
    else {
       if (bDown) {
                pFigure[0].x = nXCenter - nWidth /2;
                pFigure[0].y = nYCenter - nHeight/2;
                pFigure[1].x = nXCenter + nWidth /2;
                pFigure[1].y = nYCenter - nHeight/2;
                pFigure[2].x = nXCenter;
                pFigure[2].y = nYCenter + nHeight/2;
        } else {
                pFigure[0].x = nXCenter;
                pFigure[0].y = nYCenter - nHeight/2;
                pFigure[1].x = nXCenter - nWidth /2;
                pFigure[1].y = nYCenter + nHeight/2;
                pFigure[2].x = nXCenter + nWidth /2;
                pFigure[2].y = nYCenter + nHeight/2;
        }

    }

    Polygon(hDC, pFigure, 3);

    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);

    if( hPen   ) DeleteObject(hPen  );
    if( hBrush ) DeleteObject(hBrush);
}

