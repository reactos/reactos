
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
#include "imgwell.h"
#include "imgtools.h"
#include "t_fhsel.h"
#include "toolbox.h"
#include "imgbrush.h"
#include "imgdlgs.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif // _DEBUG

#include "memtrace.h"

BOOL  g_bUseTrans = FALSE;
BOOL  g_bCustomBrush = FALSE;
BOOL  fDraggingBrush = FALSE;
IMG*  pImgCur = NULL;

#define NUM_DEF_COLORS 28
extern COLORREF  colorColorsDef[NUM_DEF_COLORS];

COLORREF  crLeft  = 0;
COLORREF  crRight = RGB( 0xff, 0xff, 0xff );
COLORREF  crTrans = TRANS_COLOR_NONE; // transparent color
int       theLeft;
int       theRight;
int       theTrans;
int       wCombineMode;

HDC       hRubberDC;
HBITMAP   hRubberBM;
int       cxRubberWidth;
int       cyRubberHeight;
IMG*      pRubberImg;

BOOL     EnsureUndoSize(IMG* pImg);

static int   cxUndoWidth, cyUndoHeight;
static BYTE  cUndoPlanes, cUndoBitCount;

HBITMAP   g_hUndoImgBitmap = NULL;
HPALETTE  g_hUndoPalette   = NULL;

COLORREF  std2Colors [] =
    {
    RGB(000, 000, 000), //  0 - black
    RGB(255, 255, 255)  //  1 - white
    };

COLORREF  std16Colors [] =
    {
    RGB(  0,   0,   0), // Black
    RGB(128,   0,   0), // Dark Red
    RGB(  0, 128,   0), // Dark Green
    RGB(128, 128,   0), // Pea Green
    RGB(  0,   0, 128), // Dark Blue
    RGB(128,   0, 128), // Lavender
    RGB(  0, 128, 128), // Slate
    RGB(192, 192, 192), // Light Gray
    RGB(128, 128, 128), // Dark Gray
    RGB(255,   0,   0), // Bright Red
    RGB(  0, 255,   0), // Bright Green
    RGB(255, 255,   0), // Yellow
    RGB(  0,   0, 255), // Bright Blue
    RGB(255,   0, 255), // Magenta
    RGB(  0, 255, 255), // Cyan
    RGB(255, 255, 255)  //  1 - white
    };

/***************************************************************************/

IMG* CreateImg(int cxWidth, int cyHeight, int cPlanes, int cBitCount, BOOL bPalette )
{
        IMG* pimg = NULL;
        CTempBitmap bmNew;
        HBITMAP hbmOld = NULL;

        CClientDC dcScreen(NULL);


        if (! cPlanes )
        {
                cPlanes = dcScreen.GetDeviceCaps( PLANES );
        }
        if (! cBitCount)
        {
                cBitCount = dcScreen.GetDeviceCaps( BITSPIXEL );
        }

        CDC cDC;
        cDC.CreateCompatibleDC( &dcScreen );
        if (!cDC.m_hDC)
        {
                return NULL;
        }

        if (cPlanes * cBitCount > 1)
        {
                cDC.SetStretchBltMode(HALFTONE);

        }

        // Set these to 0 to not create a bitmap
        if (cxWidth && cyHeight)
        {
                BOOL bMono = (cPlanes == 1 && cBitCount == 1);
                COLORREF* pcrColors = NULL;
                int nColors = 0;

                cBitCount *= cPlanes;
                if (cBitCount <= 1)
                {
                        cBitCount = 1;
                        pcrColors = std2Colors;
                        nColors = 2;
                }
                else if (cBitCount <= 4)
                {
                        cBitCount = 4;
                        pcrColors = std16Colors;
                        nColors = 16;
                }
                else if (cBitCount <= 8)
                {
                        cBitCount = 8;
                        pcrColors = colorColorsDef;
                        nColors = NUM_DEF_COLORS;
                }
                else
                {
                        // I don't want to deal with 15 or 16 bit images
                        cBitCount = 24;
                }

                HBITMAP hbmNew = NULL;

                if (cBitCount == 4)
                {
                        // Just create a DDB if in 16 colors
                        hbmNew = CreateCompatibleBitmap( dcScreen.m_hDC, cxWidth, cyHeight);
                }
                else
                {
                        struct BMHDR
                        {
                                BITMAPINFOHEADER bmInfo;
                                RGBQUAD          rgb[256];
                        } DIBHdr =
                        {
                                sizeof(BITMAPINFOHEADER),
                                cxWidth,
                                cyHeight,
                                1,
                                (WORD)cBitCount,
                                BI_RGB,
                                0,
                                0,
                                nColors,
                                nColors,
                        } ;

                        if (cBitCount <= 8)
                        {
                                RGBQUAD* prgb;
                                COLORREF* pcr;
                                int n;

                                pcr = pcrColors;
                                prgb = DIBHdr.rgb;

                                for (n=nColors; n>0; --n, ++pcr, ++prgb)
                                {
                                        prgb->rgbRed   = GetRValue(*pcr);
                                        prgb->rgbGreen = GetGValue(*pcr);
                                        prgb->rgbBlue  = GetBValue(*pcr);
                                        prgb->rgbReserved = 0;
                                }
                        }

                        LPVOID lpNewBits;
                        hbmNew = CreateDIBSection(cDC.m_hDC, (LPBITMAPINFO)&DIBHdr,
                                DIB_RGB_COLORS, &lpNewBits, NULL, 0);
                }

                if (!hbmNew)
                {
                        return NULL;
                }
                bmNew.Attach(hbmNew);
        }

        TRY
        {
                pimg = new IMG;
        }
        CATCH (CMemoryException, e)
        {
                TRACE( TEXT("CreateImg: Can't alloc an IMG\n") );
                return NULL;
        }
        END_CATCH

        pimg->cxWidth        = cxWidth;
        pimg->cyHeight       = cyHeight;
        pimg->cPlanes        = cPlanes;
        pimg->cBitCount      = cBitCount;
        pimg->hDC            = (HDC)cDC.Detach();
        pimg->hBitmap        = (HBITMAP)bmNew.Detach();
        pimg->hBitmapOld     = NULL;
        pimg->m_pFirstImgWnd = NULL;
        pimg->bDirty         = FALSE;
        pimg->m_hPalOld      = NULL;
        pimg->m_pPalette     = NULL;

        pimg->m_bTileGrid = g_bDefaultTileGrid;
        pimg->m_cxTile    = g_defaultTileGridSize.cx;
        pimg->m_cyTile    = g_defaultTileGridSize.cy;


        BYTE cRed   = GetRValue( crRight );
        BYTE cGreen = GetGValue( crRight );
        BYTE cBlue  = GetBValue( crRight );

        if (theApp.m_bPaletted)
        {
                crRight = PALETTERGB( cRed, cGreen, cBlue );
        }
        else
        {
                crRight =        RGB( cRed, cGreen, cBlue );
        }

        if (pimg->hBitmap)
        {
                pimg->hBitmapOld = (HBITMAP)SelectObject(pimg->hDC, pimg->hBitmap);
                ClearImg( pimg );
        }

        return(pimg);
}

/***************************************************************************/

BOOL ClearImg(IMG* pimg)
    {
#if 1
    HBRUSH   hNewBrush;
    HBRUSH   hOldBrush = NULL;
    HPALETTE hpalOld   = NULL;

    pimg->m_nLastChanged = -1;

    if ((hNewBrush = ::CreateSolidBrush( crRight )) == NULL)
        return FALSE;

    if (pimg->m_pPalette)
        {
        hpalOld = SelectPalette( pimg->hDC, (HPALETTE)pimg->m_pPalette->m_hObject, FALSE );
        RealizePalette( pimg->hDC );
        }

    hOldBrush = (HBRUSH)SelectObject( pimg->hDC, hNewBrush );

    PatBlt( pimg->hDC, 0, 0, pimg->cxWidth, pimg->cyHeight, PATCOPY );

    if (hOldBrush)
        SelectObject(pimg->hDC, hOldBrush);

    DeleteObject( hNewBrush );

    if (hpalOld)
        SelectPalette( pimg->hDC, hpalOld, FALSE );

    return TRUE;
#else
        BOOL    bResult = FALSE;
        HBRUSH  hNewBrush = ::CreateSolidBrush( crRight );

        if ( hNewBrush )
                {
                HBRUSH  hOldBrush = (HBRUSH)SelectObject( pimg->hDC, hNewBrush );
                if ( hOldBrush )
                        {
                        HPALETTE hpalOld = SelectPalette( pimg->hDC,
                                                                                          (HPALETTE)pimg->m_pPalette->m_hObject,
                                                                                          FALSE );
                        if ( hpalOld )
                                {
                                RealizePalette( pimg->hDC );

                            PatBlt( pimg->hDC, 0, 0, pimg->cxWidth, pimg->cyHeight, PATCOPY );
                                pimg->m_nLastChanged = -1;
                                bResult = TRUE;

                        SelectPalette( pimg->hDC, hpalOld, FALSE );
                                }
                SelectObject(pimg->hDC, hOldBrush);
                        }
                DeleteObject( hNewBrush );
                }
        return bResult;
#endif
    }

/***************************************************************************/

void FreeImg(IMG* pimg)
    {
    if (! pimg)
        return;

    if (pimg == theImgBrush.m_pImg)
        theImgBrush.m_pImg = NULL;

    if (pimg->hDC)
        {
        if (pimg->hBitmapOld)
            SelectObject( pimg->hDC, pimg->hBitmapOld );

        if (pimg->m_hPalOld)
            SelectPalette( pimg->hDC, pimg->m_hPalOld, FALSE ); // Background ??

        DeleteDC(pimg->hDC);
        }

    if (pimg->hBitmap)
        DeleteObject(pimg->hBitmap);

    if (pimg->m_pPalette)
        delete pimg->m_pPalette;

    if (pimg->m_pBitmapObj->m_pImg == pimg)
        pimg->m_pBitmapObj->m_pImg = NULL;

    if (pImgCur == pimg)
        pImgCur = NULL;

    if (pRubberImg == pimg)
        pRubberImg = NULL;

    delete pimg;
    }

/***************************************************************************/

void SelectImg(IMG* pimg)
    {
    if (pimg == pImgCur)
        return;

    if (theImgBrush.m_pImg)
        HideBrush();

    pImgCur = pimg;

    SetupRubber(pimg);
    }

/***************************************************************************/

void DirtyImg( IMG* pimg )
    {
    CPBDoc* pDoc = (CPBDoc*)((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument();

    if (pDoc)
        {
        pDoc->SetModifiedFlag( TRUE );

        if (theApp.m_bEmbedded)
            pDoc->NotifyChanged();
        }

    pimg->bDirty = TRUE;
    pimg->m_pBitmapObj->SetDirty( TRUE );
    }

/***************************************************************************/

void CleanupImages()
    {
    FreeImg( pImgCur );

    CleanupImgUndo();
    CleanupImgRubber();
    }

/***************************************************************************/

void InvalImgRect(IMG* pimg, CRect* prc)
    {
    CImgWnd* pImgWnd;
    CImgWnd* pNextImgWnd;

    CRect rc;

    rc.SetRect(0, 0, pimg->cxWidth, pimg->cyHeight);

    if (prc)
        rc &= *prc;

    for (pImgWnd = pimg->m_pFirstImgWnd; pImgWnd;
                                         pImgWnd = pNextImgWnd)
        {
        CRect rcWnd;

        pNextImgWnd = pImgWnd->m_pNextImgWnd;

        if (prc)
            {
            rcWnd = rc;
            pImgWnd->ImageToClient(rcWnd);

            if (pImgWnd->IsGridVisible())
                {
                rcWnd.right  += 1;
                rcWnd.bottom += 1;
                }
            }

        pImgWnd->InvalidateRect(prc == NULL ? NULL : &rcWnd, FALSE);
        }
    }

/***************************************************************************/

void CommitImgRect(IMG* pimg, CRect* prc)
    {
    ASSERT(hRubberDC);

    if (hRubberDC == NULL)
        return;

    CRect rc;

    if (prc == NULL)
        {
        SetRect( &rc, 0, 0, pimg->cxWidth, pimg->cyHeight );
        prc = &rc;
        }

    HPALETTE hpalOld = NULL;

    if (theApp.m_pPalette
    &&  theApp.m_pPalette->m_hObject)
        {
        hpalOld = SelectPalette( hRubberDC, (HPALETTE)theApp.m_pPalette->m_hObject, FALSE ); // Background ??
        RealizePalette( hRubberDC );
        }

    BitBlt(hRubberDC, prc->left, prc->top,
                      prc->Width(), prc->Height(),
           pimg->hDC, prc->left, prc->top, SRCCOPY);

    if (hpalOld)
        SelectPalette( hRubberDC, hpalOld, FALSE ); // Background ??
    }

void SelInScreenFirst(CPalette* pPal)
{
        // HACK: Select into screen DC first for GDI bug
        CWindowDC hdcScreen(NULL);
        hdcScreen.SelectPalette(pPal, TRUE);
        hdcScreen.RealizePalette();
}

BOOL CreateSafePalette(CPalette* pPal, LPLOGPALETTE lpLogPal)
{
        if (!pPal->CreatePalette(lpLogPal))
        {
                return(FALSE);
        }

        SelInScreenFirst(pPal);
        return(TRUE);
}


/***************************************************************************/

BOOL ReplaceImgPalette( IMG* pImg, LPLOGPALETTE lpLogPal )
    {
    if (pImg->m_hPalOld)
        {
        ::SelectPalette( pImg->hDC, pImg->m_hPalOld, FALSE );
        pImg->m_hPalOld = NULL;
        }

    if (pImg->m_pPalette)
        delete pImg->m_pPalette;

    pImg->m_pPalette = new CPalette;

    if (pImg->m_pPalette
    &&  CreateSafePalette(pImg->m_pPalette, lpLogPal ))
        {
        pImg->m_hPalOld = ::SelectPalette( pImg->hDC,
                                 (HPALETTE)pImg->m_pPalette->GetSafeHandle(), FALSE );
        ::RealizePalette( pImg->hDC );
        InvalImgRect( pImg, NULL );
        }
    else
        {
        if (pImg->m_pPalette)
            delete pImg->m_pPalette;

        pImg->m_pPalette = NULL;
        }

    return (pImg->m_pPalette != NULL);
    }

/***************************************************************************/

void CleanupImgRubber()
    {
    if (hRubberDC)
        {
        DeleteDC(hRubberDC);
        hRubberDC = NULL;
        }

    if (hRubberBM)
        {
        DeleteObject(hRubberBM);
        hRubberBM = NULL;
        }

    pRubberImg = NULL;

    cxRubberWidth  = 0;
    cyRubberHeight = 0;
    }

/***************************************************************************/

void IdleImage()
    {
    if (g_pMouseImgWnd)
        {
        CRect rcImage(0, 0, g_pMouseImgWnd->GetImg()->cxWidth,
                            g_pMouseImgWnd->GetImg()->cyHeight);

        g_pMouseImgWnd->ImageToClient( rcImage );

        CRect rcClient;

        g_pMouseImgWnd->GetClientRect( &rcClient );

        rcClient &= rcImage;

        CPoint pt;
        GetCursorPos( &pt );

        CPoint ptClient = pt;

        g_pMouseImgWnd->ScreenToClient( &ptClient );

        if (CWnd::WindowFromPoint( pt ) != g_pMouseImgWnd
        ||  ! rcClient.PtInRect( ptClient ))
            {
            extern MTI  mti;

            CImgTool::GetCurrent()->OnLeave( g_pMouseImgWnd, &mti );

            g_pMouseImgWnd = NULL;

            if (! CImgTool::IsDragging() &&
                ::IsWindow(((CPBFrame*)theApp.m_pMainWnd)->m_statBar.m_hWnd) )
                ((CPBFrame*)theApp.m_pMainWnd)->m_statBar.ClearPosition();
            }
        }

    if (fDraggingBrush)
        {
        CPoint   pt;
        CRect    rcClient;
        CPoint   ptClient;
        CImgWnd* pImgWnd;

        GetCursorPos(&pt);

        pImgWnd = g_pDragBrushWnd;

        if (pImgWnd == NULL)
            return;

        CRect rcImage(0, 0, pImgWnd->GetImg()->cxWidth,
                            pImgWnd->GetImg()->cyHeight);

        pImgWnd->ImageToClient( rcImage   );
        pImgWnd->GetClientRect( &rcClient );

        rcClient &= rcImage;

        ptClient = pt;
        pImgWnd->ScreenToClient( &ptClient );

        if ( CWnd::WindowFromPoint(pt) != pImgWnd
        ||  ! rcClient.PtInRect(ptClient))
            {
            if (fDraggingBrush && theImgBrush.m_pImg == NULL)
                HideBrush();

            pImgWnd->UpdPos( CPoint(-1, -1) );
            }
        else
            if (GetCapture() == NULL)
                {
                CPoint imagePt = ptClient;

                pImgWnd->ClientToImage( imagePt );

                if (! g_bBrushVisible && fDraggingBrush
                &&    CImgTool::GetCurrent()->UsesBrush())
                    {
                    pImgWnd->ShowBrush( imagePt );
                    }
                }
        }
    }

/***************************************************************************/

void HideBrush()
    {
    if (! g_bBrushVisible)
        return;

    g_bBrushVisible = FALSE;

    CImgWnd* pImgWnd = g_pDragBrushWnd;

    ASSERT(pImgWnd);

    if (pImgWnd == NULL)
        return;

    IMG* pimg = pImgWnd->GetImg();

    if (pimg == NULL)
        return;

    HPALETTE hpalOld = pImgWnd->SetImgPalette( hRubberDC, FALSE ); // Background ??

    BitBlt( pimg->hDC, rcDragBrush.left,
                       rcDragBrush.top,
                       rcDragBrush.Width(),
                       rcDragBrush.Height(),
            hRubberDC, rcDragBrush.left,
                       rcDragBrush.top, SRCCOPY );

    if (hpalOld)
        SelectPalette( hRubberDC, hpalOld, FALSE ); // Background ??

    InvalImgRect( pimg, &rcDragBrush );
    }

/***************************************************************************/

void FixRect(RECT * prc)
    {
    int t;

    if (prc->left > prc->right)
        {
        t = prc->left;
        prc->left = prc->right;
        prc->right = t;
        }

    if (prc->top > prc->bottom)
        {
        t = prc->top;
        prc->top = prc->bottom;
        prc->bottom = t;
        }
    }

/***************************************************************************/

BOOL SetupRubber(IMG* pimg)
    {
    if (cxRubberWidth  < pimg->cxWidth
    ||  cyRubberHeight < pimg->cyHeight)
        {
        HBITMAP hNewBitmap;

        HideBrush();

        if (hRubberDC == NULL
        && (hRubberDC = CreateCompatibleDC( pimg->hDC )) == NULL)
            return FALSE;

        hNewBitmap = CreateCompatibleBitmap( pimg->hDC, pimg->cxWidth, pimg->cyHeight );

        if (hNewBitmap == NULL)
            {
            return FALSE;
            }

        hRubberBM = hNewBitmap;

        DeleteObject( SelectObject( hRubberDC, hRubberBM ) );

        cxRubberWidth  = pimg->cxWidth;
        cyRubberHeight = pimg->cyHeight;
        }

    if (pRubberImg != pimg)
        {
        HideBrush();

        HPALETTE hpalOld = NULL;

        if (theApp.m_pPalette
        &&  theApp.m_pPalette->m_hObject)
            {
            hpalOld = SelectPalette( hRubberDC, (HPALETTE)theApp.m_pPalette->m_hObject, FALSE );
            RealizePalette( hRubberDC );
            }

        BitBlt( hRubberDC, 0, 0, pimg->cxWidth,
                                 pimg->cyHeight, pimg->hDC, 0, 0, SRCCOPY );

        if (hpalOld)
            SelectPalette( hRubberDC, hpalOld, FALSE ); // Background ??
        }

    pRubberImg = pimg;

    return TRUE;
    }

/***************************************************************************/

CPalette* CreatePalette( const COLORREF* colors, int nColors )
    {
    CPalette* pPal = new CPalette;

    if (pPal)
        {
        LPLOGPALETTE pLogPal = (LPLOGPALETTE)new char [sizeof (LOGPALETTE) +
                                           sizeof (PALETTEENTRY) * nColors];
        if (pLogPal)
            {
            pLogPal->palVersion    = 0x300;
            pLogPal->palNumEntries = (WORD)nColors;

            for (int i = 0; i < nColors; i++)
                {
                pLogPal->palPalEntry[i] = *(PALETTEENTRY*)colors++;
                pLogPal->palPalEntry[i].peFlags = 0;
                }

            if (! CreateSafePalette(pPal, pLogPal ))
                {
                theApp.SetGdiEmergency();
                delete pPal;
                pPal = NULL;
                }

            delete [] (char*)pLogPal;
            }
        else
            {
            theApp.SetMemoryEmergency();
            delete pPal;
            pPal = NULL;
            }
        }
    return pPal;
    }

/***************************************************************************/

CPalette* GetStd16Palette()
    {
    return CreatePalette( std16Colors, 16 );
    }

/***************************************************************************/

CPalette* GetStd2Palette()
    {
    return CreatePalette( std2Colors, 2 );
    }

/***************************************************************************/

CPalette* GetStd256Palette()
    {
    return CreatePalette( colorColorsDef, NUM_DEF_COLORS );
    }

/***************************************************************************/

CPalette *PaletteFromDS(HDC hdc)
{
    DWORD adw[257];
    int i,n;
        CPalette        *pPal = new CPalette;

    if ( n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]) )
                {
            for (i=1; i<=n; i++)
                adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));

            adw[0] = MAKELONG(0x300, n);

                CreateSafePalette(pPal, (LPLOGPALETTE)&adw[0]);
                }
        else
                {
                // No Palette in Bitmap! Use default half-tone palette
                pPal->Attach(CreateHalftonePalette( NULL ));
                }

        return pPal;
}

/////////////////////////////////////////////////////////////////////////////
//
// Packed-DIB Handling Functions
//
// A packed-DIB is a bucket of bits usually consisting of a BITMAPINFOHEADER
// structure followed by an array of RGBQUAD structures followed by the words
// that make up the image.  An alternate form consists of a BITMAPCOREHEADER
// structure followed by an array of RGBTRIPLE structures and the image words.
// This format is used by OS/2, but is supported by Windows.  The only way
// to tell which format the DIB is using is to check the first word against
// the sizes of the header structures (pretty clever eh?).
//
// This is very similar to a DIB as stored in a file.  In fact, a DIB file is
// a BITMAPFILEHEADER structure followed by a packed DIB.
//
// These functions make dealing with packed-DIBs in memory easier.
//
#define WIDTHBYTES(bits) ((((bits) + 31) / 32) * 4)

/***************************************************************************/

void FreeDib(LPSTR lpDib)
    {
    ASSERT( lpDib );

    if (lpDib)
        delete [] (CHAR*)lpDib;
    }

/***************************************************************************/

LPSTR DibFromBitmap( HBITMAP hBitmap, DWORD dwStyle, WORD wBits,
                     CPalette* pPal, HBITMAP hMaskBitmap, DWORD& dwLen )
    {
    ASSERT(hBitmap);

    if (hBitmap == NULL)
        return NULL;

     ASSERT(hMaskBitmap == NULL || dwStyle == BI_RGB);

    HBITMAP            hbm;
    BITMAP             bm;
    BITMAPINFOHEADER   bi;
    LPBITMAPINFOHEADER lpbi;
    HDC                hDC;
    DWORD              dwSmallLen;


    HPALETTE hPal = (HPALETTE)(pPal->GetSafeHandle());

    if (theApp.m_bPaletted && ! hPal)
        hPal = (HPALETTE)::GetStockObject( DEFAULT_PALETTE );

    GetObject( hBitmap, sizeof( bm ), (LPSTR)&bm );

    if (wBits == 0)
        wBits = bm.bmPlanes * bm.bmBitsPixel;

    if (wBits <= 1)
        wBits = 1;
    else
        if (wBits <= 4)
            wBits = 4;
        else
            if (wBits <= 8)
                wBits = 8;
            else
                wBits = 24;

    bi.biSize          = sizeof( BITMAPINFOHEADER );
    bi.biWidth         = bm.bmWidth;
    bi.biHeight        = bm.bmHeight;
    bi.biPlanes        = 1;
    bi.biBitCount      = wBits;
    bi.biCompression   = dwStyle;
    bi.biSizeImage     = 0;
//  bi.biXPelsPerMeter = theApp.ScreenDeviceInfo.ixPelsPerDM * 10;
//  bi.biYPelsPerMeter = theApp.ScreenDeviceInfo.iyPelsPerDM * 10;
    HDC hdc = GetDC(NULL);
    bi.biXPelsPerMeter = MulDiv(::GetDeviceCaps(hdc, LOGPIXELSX),10000, 254);
    bi.biYPelsPerMeter = MulDiv(::GetDeviceCaps(hdc, LOGPIXELSY),10000, 254);
    ReleaseDC (NULL, hdc);

    bi.biClrUsed       = 0;
    bi.biClrImportant  = 0;

    dwSmallLen = dwLen = bi.biSize + PaletteSize( (LPSTR) &bi );

    lpbi = (LPBITMAPINFOHEADER)new CHAR[dwLen];

    memset( (void*)lpbi, 0, dwLen );

    if (lpbi == NULL)
        {
        theApp.SetMemoryEmergency();
        return NULL;
        }

    *lpbi = bi;

    hbm = CreateBitmap( 2, 2, bm.bmPlanes, bm.bmBitsPixel, NULL );
    hDC = CreateCompatibleDC( NULL );

    if (hbm == NULL || hDC == NULL)
        {
        if (hbm)
            DeleteObject( hbm );

        theApp.SetGdiEmergency();
        return NULL;
        }
    HPALETTE hPalOld = NULL;
    HANDLE   hbmOld  = SelectObject( hDC, hbm );

    if (hPal)
        {
        hPalOld = SelectPalette( hDC, hPal, FALSE );
        RealizePalette( hDC );
        }

    // Compute the byte size of the DIB...
    GetDIBits( hDC, hBitmap, 0, (WORD)bi.biHeight, NULL, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS );

    bi = *lpbi;

    // If the driver did not fill in the biSizeImage field, make one up
    // NOTE: This size will be too big if the bitmap is compressed!
    // NOTE: This happens with the Paradise 800x600x256 driver...
    if (bi.biSizeImage == 0)
        {
        TRACE( TEXT("Display driver bug!  We have to compute DIB size...") );

        bi.biSizeImage = WIDTHBYTES( (DWORD)bi.biWidth * wBits ) * bi.biHeight;

        if (dwStyle != BI_RGB)
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
        }

    dwLen = bi.biSize + PaletteSize( (LPSTR)&bi ) + bi.biSizeImage;

    if (hMaskBitmap)
        dwLen += (LONG)WIDTHBYTES( bi.biWidth ) * bi.biHeight;

    CHAR* hpv = new CHAR[dwLen];

    memset( hpv, 0, dwLen );

    if (! hpv)
        {
        theApp.SetMemoryEmergency();

        delete [] (CHAR*)lpbi;

        if (hbmOld)
            DeleteObject( SelectObject( hDC, hbmOld ) );

        if (hPalOld)
            SelectPalette( hDC, hPalOld, FALSE );

        DeleteDC( hDC );

        return NULL;
        }

    memcpy( hpv, (void*)lpbi, dwSmallLen );

    delete [] (CHAR*)lpbi;

    lpbi = (LPBITMAPINFOHEADER)hpv;

    LPSTR lpBits = (LPSTR)lpbi + lpbi->biSize + PaletteSize((LPSTR)lpbi);
    DWORD biSizeImage = lpbi->biSizeImage;

    if (hMaskBitmap)
        {
        // Do the mask first so the dib ends up with the main bitmap's
        // size and palette when we're done...
        LONG cbAdjust     = ((LONG)WIDTHBYTES( bi.biWidth * wBits )) * bi.biHeight;
              lpBits     += cbAdjust;
        WORD biBitCount   = lpbi->biBitCount;
        lpbi->biBitCount  = 1;
        lpbi->biSizeImage = 0;

        if (GetDIBits( hDC, hMaskBitmap, 0, (WORD)bi.biHeight, lpBits,
                            (LPBITMAPINFO)lpbi, DIB_RGB_COLORS ) == 0)
            {
            delete [] hpv;

            if (hbmOld)
                DeleteObject( SelectObject( hDC, hbmOld ) );

            if (hPalOld)
                SelectPalette( hDC, hPalOld, FALSE );

            DeleteDC(hDC);

            return NULL;
            }

        biSizeImage     += lpbi->biSizeImage;
        lpbi->biBitCount = biBitCount;
        lpBits          -= cbAdjust;
        }

    if (GetDIBits( hDC, hBitmap, 0, (WORD)bi.biHeight, lpBits,
                    (LPBITMAPINFO)lpbi, DIB_RGB_COLORS ) == 0)
        {
        delete [] hpv;

        if (hbmOld)
            DeleteObject( SelectObject( hDC, hbmOld ) );

        if (hPalOld)
            SelectPalette( hDC, hPalOld, FALSE );

        DeleteDC(hDC);

        return NULL;
        }

    lpbi->biSizeImage = biSizeImage;

    if (hMaskBitmap)
        lpbi->biHeight *= 2;

    if (hbmOld)
        DeleteObject( SelectObject( hDC, hbmOld ) );

    if (hPalOld)
        SelectPalette( hDC, hPalOld, FALSE );
    DeleteDC( hDC );

    return (LPSTR)lpbi;
    }

/***************************************************************************/

UINT DIBBitsPixel(LPSTR lpbi)
{
        // Calculate the number of colors in the color table based on
        //  the number of bits per pixel for the DIB.

        if (IS_WIN30_DIB(lpbi))
        {
                return(((LPBITMAPINFOHEADER)lpbi)->biBitCount);
        }
        else
        {
                return(((LPBITMAPCOREHEADER)lpbi)->bcBitCount);
        }
}

WORD DIBNumColors(LPSTR lpbi, BOOL bJustUsed)
    {
    WORD wBitCount;

    // If this is a Windows style DIB, the number of colors in the
    //  color table can be less than the number of bits per pixel
    //  allows for (i.e. lpbi->biClrUsed can be set to some value).
    //  If this is the case, return the appropriate value.

    if (IS_WIN30_DIB( lpbi ) && bJustUsed)
        {
        DWORD dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;

        if (dwClrUsed != 0)
            return (WORD)dwClrUsed;
        }


    // Calculate the number of colors in the color table based on
    //  the number of bits per pixel for the DIB.

    wBitCount = (WORD)DIBBitsPixel(lpbi);

    switch (wBitCount)
        {
        case 1:
            return 2;

        case 4:
            return 16;

        case 8:
            return 256;

        default:
            return 0;
        }
    }


/***************************************************************************/

WORD PaletteSize(LPSTR lpbi)
    {


    if (IS_WIN30_DIB(lpbi) &&
                ((LPBITMAPINFOHEADER)lpbi)->biCompression==BI_BITFIELDS)
    {
            // Images with bitfields have 3 DWORD's that specify the RGB components
            // (respectively) of each pixel.
            if (((LPBITMAPINFOHEADER)lpbi)->biSize >= sizeof(BITMAPV4HEADER))
            {
               return 0;
            }
            else
               return(3 * sizeof(DWORD));
    }

    return DIBNumColors(lpbi,TRUE) *
        (IS_WIN30_DIB(lpbi) ? sizeof(RGBQUAD) : sizeof(RGBTRIPLE));
    }


/***************************************************************************/

LPSTR FindDIBBits(LPSTR lpbi, DWORD dwOffBits)
    {
    DWORD dwAfterHdr = *(LPDWORD)lpbi + PaletteSize(lpbi);
    DWORD dwOff;
#if 0
        if (dwOffBits && dwAfterHdr != dwOffBits)
        {
                MessageBeep(0);
        }
#endif
    dwOff = max(dwOffBits, dwAfterHdr);
    return(lpbi + dwOff);
    }


/***************************************************************************/

DWORD DIBWidth(LPSTR lpDIB)
    {

    LPBITMAPINFOHEADER lpbmi = (LPBITMAPINFOHEADER)lpDIB;
    LPBITMAPCOREHEADER lpbmc = (LPBITMAPCOREHEADER)lpDIB;

    if (lpbmi->biSize >= sizeof (BITMAPINFOHEADER))
        return lpbmi->biWidth;
    else
        return (DWORD)lpbmc->bcWidth;
    }


/***************************************************************************/

DWORD DIBHeight(LPSTR lpDIB)
    {
    LPBITMAPINFOHEADER lpbmi = (LPBITMAPINFOHEADER)lpDIB;
    LPBITMAPCOREHEADER lpbmc = (LPBITMAPCOREHEADER)lpDIB;

    if (lpbmi->biSize >= sizeof (BITMAPINFOHEADER))
        return lpbmi->biHeight;
    else
        return (DWORD)lpbmc->bcHeight;
    }


/***************************************************************************/

HBITMAP DIBToBitmap( LPSTR lpDIBHdr, CPalette* pPal, HDC hdc )
    {
    ASSERT( lpDIBHdr );
    ASSERT( hdc );

    if (! lpDIBHdr || ! hdc)
        return NULL;

    LPBYTE    lpDIBBits = (LPBYTE)FindDIBBits( lpDIBHdr,0 );
    CPalette* ppalOld = NULL;
    CBitmap*  pbmOld  = NULL;
    CBitmap   bmTemp;
    CDC       dc;

    dc.Attach( hdc );

    if (bmTemp.CreateCompatibleBitmap( &dc, 2, 2 ))
        pbmOld = dc.SelectObject( &bmTemp );

    if (pPal)
        {
        ASSERT( pPal->m_hObject );

#ifdef FORCEBACKPALETTE
        ppalOld = dc.SelectPalette( pPal, TRUE );
#else
        ppalOld = dc.SelectPalette( pPal, FALSE );
#endif

        dc.RealizePalette();
        }

    HBITMAP hBitmap = CreateDIBitmap( dc.m_hDC, (LPBITMAPINFOHEADER)lpDIBHdr,
                                      CBM_INIT, lpDIBBits,
                                      (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS );
    if (ppalOld)
        dc.SelectPalette( ppalOld, FALSE );

    if (pbmOld)
        dc.SelectObject( pbmOld );

    if (bmTemp.m_hObject)
        bmTemp.DeleteObject();

    dc.Detach();

    return hBitmap;
    }

/***************************************************************************/

BOOL ShouldUseDDB(HDC hdc, LPBITMAPINFO lpDIBHdr)
{
        if (!IS_WIN30_DIB(lpDIBHdr))
        {
                // I don't want to write special code to deal with this case
                return(FALSE);
        }

        if (lpDIBHdr->bmiHeader.biPlanes*lpDIBHdr->bmiHeader.biBitCount != 4)
        {
                // No DDB for mono or 8bit or more
                return(FALSE);
        }

        UINT cBitCount = GetDeviceCaps( hdc, BITSPIXEL )
                * GetDeviceCaps( hdc, PLANES );
        if (cBitCount > 4)
        {
                return(FALSE);
        }

        RGBQUAD *lpPal = lpDIBHdr->bmiColors;

        for (int i=DIBNumColors((LPSTR)lpDIBHdr, TRUE); i>0; --i, ++lpPal)
        {
                COLORREF cr = RGB(lpPal->rgbRed, lpPal->rgbGreen, lpPal->rgbBlue);
                if (GetNearestColor(hdc, cr) != cr)
                {
                        return(FALSE);
                }
        }

        // OK, so this is a WIN30 DIB, the screen is 16 or less colors, it is
        // either an uncompressed or RLE DIB, and all the colors in the DIB can
        // be shown on the screen.  I guess we can use a DDB.
        return(TRUE);
}

/***************************************************************************/


HBITMAP DIBToDS( LPSTR lpDIB, DWORD dwOffBits, HDC hdc )
{
        ASSERT( lpDIB );
        ASSERT( hdc );

        if (! lpDIB || ! hdc)
        {
                return NULL;
        }

        LPVOID lpNewBits;
        LPBITMAPINFO lpDIBHdr = (LPBITMAPINFO)lpDIB;
        LPBYTE lpDIBBits = (LPBYTE)FindDIBBits( lpDIB, dwOffBits );

        {
                // New block just to scope dcScreen
                CClientDC dcScreen(NULL);

                if (ShouldUseDDB(dcScreen.m_hDC, lpDIBHdr))
                {
                        return(CreateDIBitmap( dcScreen.m_hDC, &lpDIBHdr->bmiHeader,
                                CBM_INIT, lpDIBBits, lpDIBHdr, DIB_RGB_COLORS ));
                }
        }

        // Compressed DIB sections are not allowed
        DWORD dwCompression = lpDIBHdr->bmiHeader.biCompression;
        if (IS_WIN30_DIB(lpDIB))
        {
                lpDIBHdr->bmiHeader.biCompression = BI_RGB;
        }
        HBITMAP hBitmap = CreateDIBSection( hdc, lpDIBHdr, DIB_RGB_COLORS,
                &lpNewBits, NULL, 0);

        if (IS_WIN30_DIB(lpDIB))
        {
                lpDIBHdr->bmiHeader.biCompression = dwCompression;
        }

        if (hBitmap)
        {
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdc, hBitmap);
                if (hbmOld)
                {
                        UINT uWid = DIBWidth(lpDIB);
                        UINT uHgt = DIBHeight(lpDIB);

                        // Fill with white in case the bitmap has any jumps in it.
                        PatBlt(hdc, 0, 0, uWid, uHgt, WHITENESS);

//                        StretchDIBits(hdc, 0, 0, uWid, uHgt, 0, 0, uWid, uHgt, lpDIBBits,
//                                (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS, SRCCOPY);
                        SetDIBitsToDevice (hdc,0,0,uWid, uHgt,0,0,0,abs(uHgt),lpDIBBits,
                                           (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS);
                        SelectObject(hdc, hbmOld);

                        return(hBitmap);
                }

                DeleteObject(hBitmap);
        }

        return(NULL);
}

//------------------------------------------------------------------------------
// SetNewPalette - Used solely by GetRainbowPalette below for initialization.
//------------------------------------------------------------------------------

static void SetNewPalette( PALETTEENTRY* const pPal, PWORD pwRainbowColors,
                                                     UINT R, UINT G, UINT B )
    {
    if (*pwRainbowColors < 256)
        {
        WORD wC;

        for (wC = 0;  wC < *pwRainbowColors;  wC++)
            if (((UINT)GetRValue( *(DWORD*)&pPal[ wC ] ) /* + 1 & ~1 */ ) == (R /* + 1 & ~1 */ )
            &&  ((UINT)GetGValue( *(DWORD*)&pPal[ wC ] ) /* + 1 & ~1 */ ) == (G /* + 1 & ~1 */ )
            &&  ((UINT)GetBValue( *(DWORD*)&pPal[ wC ] ) /* + 1 & ~1 */ ) == (B /* + 1 & ~1 */ ))
                return;

        pPal[*pwRainbowColors].peRed   = (BYTE)R;
        pPal[*pwRainbowColors].peGreen = (BYTE)G;
        pPal[*pwRainbowColors].peBlue  = (BYTE)B;
        pPal[*pwRainbowColors].peFlags = 0;

        (*pwRainbowColors)++;
        }
    }

/***************************************************************************/

CPalette* CreateDIBPalette(LPSTR lpbi)
    {
    LPLOGPALETTE     lpPal;
    CPalette*        pPal = NULL;
    int              iLoop;
    int              wNumColors;
    LPBITMAPINFO     lpbmi;
    LPBITMAPCOREINFO lpbmc;
    BOOL             bWinStyleDIB;
    BOOL             bGetDriverDefaults = FALSE;

    ASSERT( lpbi );

    if (lpbi == NULL)
        return NULL;

    lpbmi = (LPBITMAPINFO)lpbi;
    lpbmc = (LPBITMAPCOREINFO)lpbi;
    wNumColors = (int)DIBNumColors(lpbi, TRUE);
    bWinStyleDIB = IS_WIN30_DIB(lpbi);

    if (! wNumColors)
        {
        if (! theApp.m_bPaletted)
            return NULL;

        bGetDriverDefaults = TRUE;
        wNumColors = 256;
        }

    lpPal = (LPLOGPALETTE)new CHAR [sizeof( LOGPALETTE ) + sizeof( PALETTEENTRY ) * (wNumColors - 1)];

    if (lpPal == NULL)
        {
        theApp.SetMemoryEmergency();
        return NULL;
        }

    if (bGetDriverDefaults)
        {
        #define DIM( X ) (sizeof(X) / sizeof(X[0]))

        // GetRainbowPalette - Based on
        // Fran Finnegan's column in Microsoft Systems Journal, Sept.-Oct., 1991 (#5).
        static BYTE C[] = { 255, 238, 221,
                            204, 187, 170,
                            153, 136, 119,
                            102,  85,  68,
                             51,  34,  17,
                              0
                          };
        PALETTEENTRY* pPal = &(lpPal->palPalEntry[0]);
        WORD wColors = 0;
        int iC;
        int iR;
        int iG;
        int iB;

        for (iC = 0;  iC < DIM( C );  iC++)
            SetNewPalette( pPal, &wColors, C[ iC ], C[ iC ], C[ iC ] );
        for (iR = 0;  iR < DIM( C );  iR += 3)
        for (iG = 0;  iG < DIM( C );  iG += 3)
        for (iB = 0;  iB < DIM( C );  iB += 3)
            SetNewPalette( pPal, &wColors, C[ iR ], C[ iG ], C[ iB ] );
        for (iC = 0;  iC < DIM( C );  iC++)
            {
            SetNewPalette( pPal, &wColors, C[ iC ],       0,       0 );
            SetNewPalette( pPal, &wColors,       0, C[ iC ],       0 );
            SetNewPalette( pPal, &wColors,       0,       0, C[ iC ] );
            }
        }
    else
        for (iLoop = 0; iLoop < wNumColors; iLoop++)
            {
            if (bWinStyleDIB)
                {
                lpPal->palPalEntry[iLoop].peRed   = lpbmi->bmiColors[iLoop].rgbRed;
                lpPal->palPalEntry[iLoop].peGreen = lpbmi->bmiColors[iLoop].rgbGreen;
                lpPal->palPalEntry[iLoop].peBlue  = lpbmi->bmiColors[iLoop].rgbBlue;
                lpPal->palPalEntry[iLoop].peFlags = 0;
                }
            else
                {
                lpPal->palPalEntry[iLoop].peRed   = lpbmc->bmciColors[iLoop].rgbtRed;
                lpPal->palPalEntry[iLoop].peGreen = lpbmc->bmciColors[iLoop].rgbtGreen;
                lpPal->palPalEntry[iLoop].peBlue  = lpbmc->bmciColors[iLoop].rgbtBlue;
                lpPal->palPalEntry[iLoop].peFlags = 0;
                }
            }
    lpPal->palVersion = 0x300;
    lpPal->palNumEntries = (WORD)wNumColors;

    pPal = new CPalette;

    if (pPal == NULL || ! CreateSafePalette(pPal, lpPal ))
        {
        if (pPal)
            delete pPal;
        pPal = NULL;
        }

    delete [] (CHAR*)lpPal;

    return pPal;
    }

/***************************************************************************/

void Draw3dRect(HDC hDC, RECT * prc)
    {
    CDC* pDC = CDC::FromHandle(hDC);
    CBrush* pOldBrush = pDC->SelectObject( GetSysBrush( COLOR_BTNSHADOW ) );

    pDC->PatBlt(prc->left, prc->top, prc->right - prc->left - 1, 1, PATCOPY);
    pDC->PatBlt(prc->left, prc->top + 1, 1, prc->bottom - prc->top - 2, PATCOPY);

    pDC->SelectObject(GetSysBrush( COLOR_BTNHIGHLIGHT ));

    pDC->PatBlt(prc->left + 1, prc->bottom - 1,
                               prc->right - prc->left - 1, 1, PATCOPY);
    pDC->PatBlt(prc->right - 1, prc->top + 1,
                             1, prc->bottom - prc->top - 1, PATCOPY);

    pDC->SelectObject(pOldBrush);
    }

/***************************************************************************/
// DrawBitmap:
// See header file for usage.

void DrawBitmap(CDC* dc, CBitmap* bmSrc, CRect* rc,
    DWORD dwROP /* = SRCCOPY */, CDC* memdc /* = NULL */)
    {
    CBitmap* obm;
    CDC* odc = (memdc? memdc : (new CDC));

    if (!memdc)
        odc->CreateCompatibleDC(dc);

    obm = odc->SelectObject(bmSrc);

    BITMAP bms;
    bmSrc->GetObject(sizeof (BITMAP), (LPSTR)(LPBITMAP)(&bms));

    if (rc)
        {
        dc->BitBlt(rc->left + ((rc->right - rc->left - bms.bmWidth) >> 1),
            rc->top + ((rc->bottom - rc->top - bms.bmHeight) >> 1),
            bms.bmWidth, bms.bmHeight, odc, 0, 0, dwROP);
        }
    else
        {
        dc->BitBlt(0, 0, bms.bmWidth, bms.bmHeight, odc, 0, 0, dwROP);
        }

    odc->SelectObject(obm);
    if (!memdc)
        {
        odc->DeleteDC();
        delete odc;
        }
    }

/***************************************************************************/

BOOL EnsureUndoSize(IMG* pimg)
    {
    if (cxUndoWidth < pimg->cxWidth || cyUndoHeight < pimg->cyHeight ||
            (int)cUndoPlanes != pimg->cPlanes ||
            (int)cUndoBitCount != pimg->cBitCount)
        {
        HBITMAP  hNewUndoImgBitmap  = NULL;
 //       HBITMAP  hNewUndoMaskBitmap = NULL;
        HPALETTE hNewUndoPalette    = NULL;

        hNewUndoImgBitmap = CreateCompatibleBitmap( pimg->hDC, pimg->cxWidth, pimg->cyHeight );

        if (hNewUndoImgBitmap == NULL)
            {
            TRACE(TEXT("EnsureUndoSize: Create image bitmap failed!\n"));
            return FALSE;
            }

//        hNewUndoMaskBitmap = CreateBitmap( pimg->cxWidth, pimg->cyHeight, 1, 1, NULL );

//        if (hNewUndoMaskBitmap == NULL)
 //           {
//            TRACE(TEXT("EnsureUndoSize: Create mask bitmap failed!\n"));

//            DeleteObject( hNewUndoImgBitmap );

 //           return FALSE;
   //         }

//      if (theApp.m_pPalette)
//          {
//          LOGPALETTE256 logPal;

//          logPal.palVersion    = 0x300;
//          logPal.palNumEntries = theApp.m_pPalette->GetPaletteEntries( 0, 256,
//                                                                &logPal.palPalEntry[0] );
//          theApp.m_pPalette->GetPaletteEntries( 0, logPal.palNumEntries,
//                                                                &logPal.palPalEntry[0] );
//          hNewUndoPalette = ::CreatePalette( (LPLOGPALETTE)&logPal );

//          if (! hNewUndoPalette)
//              {
//              TRACE("EnsureUndoSize: Create palette bitmap failed!\n");

//              DeleteObject( hNewUndoImgBitmap  );
//              DeleteObject( hNewUndoMaskBitmap );

//              return FALSE;
//              }
//          }

        if (g_hUndoImgBitmap)
            DeleteObject(g_hUndoImgBitmap);

//      if (g_hUndoPalette)
//          DeleteObject(g_hUndoPalette);

        g_hUndoImgBitmap = hNewUndoImgBitmap;
//      g_hUndoPalette   = hNewUndoPalette;
        cxUndoWidth      = pimg->cxWidth;
        cyUndoHeight     = pimg->cyHeight;
        cUndoPlanes      = (BYTE)pimg->cPlanes;
        cUndoBitCount    = (BYTE)pimg->cBitCount;
        }

    return TRUE;
    }

/***************************************************************************/

void CleanupImgUndo()
    {
    if (g_hUndoImgBitmap)
        DeleteObject(g_hUndoImgBitmap);

    if (g_hUndoPalette)
        DeleteObject(g_hUndoPalette);

    g_hUndoImgBitmap = NULL;
    g_hUndoPalette   = NULL;
    cxUndoWidth      = 0;
    cyUndoHeight     = 0;
    cUndoPlanes      = 0;
    cUndoBitCount    = 0;
    }

/***************************************************************************/

BOOL SetUndo(IMG* pimg)
    {
    BOOL    bSuccess   = FALSE;
    HDC     hTempDC    = NULL;
    HBITMAP hOldBitmap = NULL;
    CRect   rect;

    if (! EnsureUndoSize( pimg ))
        goto LReturn;

    rect.SetRect( 0, 0, pimg->cxWidth, pimg->cyHeight );

    hTempDC = CreateCompatibleDC( pimg->hDC );

    if (hTempDC == NULL)
        {
        TRACE( TEXT("SetUndo: CreateCompatibleDC failed\n") );
        goto LReturn;
        }

    hOldBitmap = (HBITMAP)SelectObject( hTempDC, g_hUndoImgBitmap );

    BitBlt( hTempDC, 0, 0, rect.Width(), rect.Height(),
                pimg->hDC, rect.left, rect.top, SRCCOPY );

    SelectObject( hTempDC, hOldBitmap );
    DeleteDC( hTempDC );

    bSuccess = TRUE;

    LReturn:

    return bSuccess;
    }

/***************************************************************************/

void DrawBrush(IMG* pimg, CPoint pt, BOOL bDraw)
    {
    int nStrokeWidth = CImgTool::GetCurrent()->GetStrokeWidth();

    if (g_bCustomBrush)
        {
        CRect rc(pt.x, pt.y,
                pt.x + theImgBrush.m_size.cx, pt.y + theImgBrush.m_size.cy);
        rc -= (CPoint)theImgBrush.m_handle;

        theImgBrush.m_rcSelection = rc;

        int nCombineMode;
        COLORREF cr;

        if (bDraw)
            {
            nCombineMode = (theImgBrush.m_bOpaque) ? combineReplace : combineMatte;
            cr = crLeft;
            }
        else
            {
            nCombineMode = combineColor;
            cr = crRight;
            }

        if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
            {
//          extern CTextTool  g_textTool;
//          g_textTool.Render(CDC::FromHandle(pimg->hDC), rc, TRUE, TRUE);
            }
        else
            {
            switch (nCombineMode)
                {
#ifdef DEBUG
                default:
                    ASSERT(FALSE);
#endif

                case combineColor:
                    theImgBrush.BltColor(pimg, rc.TopLeft(), cr);
                    break;

                case combineMatte:
                    theImgBrush.BltMatte(pimg, rc.TopLeft());
                    break;

                case combineReplace:
                    theImgBrush.BltReplace(pimg, rc.TopLeft());
                    break;
                }
            }

            InvalImgRect (pimg, &rc);
            CommitImgRect(pimg, &rc);
        }
    else
        {
        DrawImgLine(pimg, pt, pt, bDraw ? crLeft : crRight,
                    CImgTool::GetCurrent()->GetStrokeWidth(),
                    CImgTool::GetCurrent()->GetStrokeShape(), TRUE);
        rcDragBrush.left = pt.x - nStrokeWidth / 2;
        rcDragBrush.top = pt.y - nStrokeWidth / 2;
        rcDragBrush.right = rcDragBrush.left + nStrokeWidth;
        rcDragBrush.bottom = rcDragBrush.top + nStrokeWidth;
        }
    }

/***************************************************************************/

void DrawDCLine(HDC hDC, CPoint pt1, CPoint pt2,
                 COLORREF color, int nWidth, int nShape,
                 CRect& rc)
{
        HPEN   hOldPen;
        HBRUSH hBrush;
        HBRUSH hOldBrush;
        int    sx;
        int    sy;
        int    ex;
        int    ey;
        int    nWidthD2 = nWidth / 2;

        sx = pt1.x;
        sy = pt1.y;
        ex = pt2.x;
        ey = pt2.y;

        if (hDC)
        {
                hBrush    = ::CreateSolidBrush( color );
                hOldBrush = (HBRUSH)SelectObject( hDC, hBrush );

                if (nWidth == 1)
                {
                        HPEN hPen = CreatePen(PS_SOLID, 1, (COLORREF)color);
                        hOldPen = (HPEN)SelectObject(hDC, hPen);

                        ::MoveToEx(hDC, sx, sy, NULL);
                        LineTo(hDC, ex, ey);
                        SetPixel(hDC, ex, ey, color);
                        SelectObject(hDC, hOldPen);
                        DeleteObject(hPen);
                }
                else
                {
//                      hOldPen = (HPEN)SelectObject(hDC, GetStockObject( NULL_PEN ));

                        BrushLine( CDC::FromHandle(hDC),
                                CPoint(sx - nWidthD2, sy - nWidthD2),
                                CPoint(ex - nWidthD2, ey - nWidthD2), nWidth, nShape);
//                      SelectObject( hDC, hOldPen );
                }

                SelectObject(hDC, hOldBrush);
                DeleteObject(hBrush);
        }

        if (sx < ex)
        {
                rc.left = sx;
                rc.right = ex + 1;
        }
        else
        {
                rc.left = ex;
                rc.right = sx + 1;
        }

        if (sy < ey)
        {
                rc.top = sy;
                rc.bottom = ey + 1;
        }
        else
        {
                rc.top = ey;
                rc.bottom = sy + 1;
        }

        rc.left   -= nWidth * 2;
        rc.top    -= nWidth * 2;
        rc.right  += nWidth * 2;
        rc.bottom += nWidth * 2;
}


void DrawImgLine(IMG* pimg, CPoint pt1, CPoint pt2,
                 COLORREF color, int nWidth, int nShape,
                 BOOL bCommit)
{
        CRect  rc;

        DrawDCLine(pimg->hDC, pt1, pt2, color, nWidth, nShape, rc);

        InvalImgRect(pimg, &rc);

        if (bCommit)
        {
                CommitImgRect(pimg, &rc);
        }
}

/***************************************************************************/

void FillImgRect( HDC hDC, CRect* prc, COLORREF cr )
    {
    FixRect( prc );

    CPoint pt1 = prc->TopLeft();
    CPoint pt2 = prc->BottomRight();

    StandardiseCoords( &pt1, &pt2 );

    int sx = pt1.x;
    int sy = pt1.y;
    int ex = pt2.x;
    int ey = pt2.y;

    CRect rc( sx, sy, ex, ey );

    HBRUSH hBr = ::CreateSolidBrush( cr );

    if (! hBr)
        {
        theApp.SetGdiEmergency();
        return;
        }

    if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        if (theImgBrush.m_cRgnPolyFreeHandSel.GetSafeHandle())
            {
            // offset bitmap in the imgwnd from selection boundary
            theImgBrush.m_cRgnPolyFreeHandSel.OffsetRgn( rc.left, rc.top );

            ::FillRgn( hDC, (HRGN)theImgBrush.m_cRgnPolyFreeHandSel.m_hObject,
                                                                        hBr );
            // offset back to selection boundary
            theImgBrush.m_cRgnPolyFreeHandSel.OffsetRgn( -rc.left, -rc.top );
            }
        }
    else
        {

        HPEN hPen = CreatePen( PS_NULL, 0, 0 );

        if (hPen)
            {
            HPEN   hOldPen =   (HPEN)SelectObject( hDC, hPen );
            HBRUSH hOldBr  = (HBRUSH)SelectObject( hDC, hBr );

            Rectangle( hDC, sx, sy, ex, ey );

            SelectObject( hDC, hOldPen);
            DeleteObject( hPen );
            SelectObject( hDC, hOldBr );
            }
        }

    DeleteObject( hBr );

    *prc = rc;
    }

/***************************************************************************/

void BrushLine(CDC* pDC, CPoint fromPt, CPoint toPt, int nWidth, int nShape)
    {
    CPen* pOldPen = (CPen*) pDC->SelectStockObject(NULL_PEN);
    CSize brushSize(nWidth, nWidth);

    int octant;

    if (nShape == slantedRightBrush || nShape == slantedLeftBrush)
        {
        int dx = abs(toPt.x - fromPt.x);
        int dy = abs(toPt.y - fromPt.y);

        if (toPt.x > fromPt.x)
            {
            if (toPt.y < fromPt.y)
                octant = (dx > dy) ? 0 : 1;
            else
                octant = (dx > dy) ? 7 : 6;
            }
        else
            {
            if (toPt.y < fromPt.y)
                octant = (dx > dy) ? 3 : 2;
            else
                octant = (dx > dy) ? 4 : 5;
            }
        }

    switch (nShape)
        {
        case squareBrush:
            PolyTo(pDC, fromPt, toPt, brushSize);
            break;

        case roundBrush:
            if (toPt != fromPt)
                {
                CRect tan;
                CPoint polyPts [4];

                if (!GetTanPt(brushSize, (CPoint)(toPt - fromPt), tan))
                    return;

                polyPts[0].x = fromPt.x + tan.left;
                polyPts[1].x = fromPt.x + tan.right;
                polyPts[2].x = toPt.x + tan.right;
                polyPts[3].x = toPt.x + tan.left;

                polyPts[0].y = fromPt.y + tan.top;
                polyPts[1].y = fromPt.y + tan.bottom;
                polyPts[2].y = toPt.y + tan.bottom;
                polyPts[3].y = toPt.y + tan.top;

                pDC->Polygon(polyPts, 4);

                Mylipse(pDC->m_hDC, fromPt.x, fromPt.y,
                        fromPt.x + brushSize.cx, fromPt.y + brushSize.cy, TRUE);
                }

            Mylipse(pDC->m_hDC, toPt.x, toPt.y,
                    toPt.x + brushSize.cx, toPt.y + brushSize.cy, TRUE);
            break;

        case slantedLeftBrush:
            {
            CPoint polyPts [6];

            fromPt.x -= 1;
            toPt.x -= 1;

            switch (octant)
                {
                case 0:
                    polyPts[0].x = fromPt.x;
                    polyPts[1].x = fromPt.x + 1;
                    polyPts[2].x = toPt.x + 1;
                    polyPts[3].x = toPt.x + brushSize.cy + 1;
                    polyPts[4].x = toPt.x + brushSize.cy;
                    polyPts[5].x = fromPt.x + brushSize.cy;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = toPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = fromPt.y;
                    break;

                case 1:
                    polyPts[0].x = fromPt.x;
                    polyPts[1].x = fromPt.x + 1;
                    polyPts[2].x = fromPt.x + brushSize.cy + 1;
                    polyPts[3].x = toPt.x + brushSize.cy + 1;
                    polyPts[4].x = toPt.x + brushSize.cy;
                    polyPts[5].x = toPt.x;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = fromPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = toPt.y + brushSize.cy;
                    break;

                case 2:
                case 3:
                    polyPts[0].x = fromPt.x + 1;
                    polyPts[1].x = fromPt.x;
                    polyPts[2].x = toPt.x;
                    polyPts[3].x = toPt.x + brushSize.cy;
                    polyPts[4].x = toPt.x + brushSize.cy + 1;
                    polyPts[5].x = fromPt.x + brushSize.cy + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = toPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = fromPt.y;
                    break;

                case 4:
                    polyPts[0].x = fromPt.x + brushSize.cy + 1;
                    polyPts[1].x = fromPt.x + brushSize.cy;
                    polyPts[2].x = toPt.x + brushSize.cy;
                    polyPts[3].x = toPt.x;
                    polyPts[4].x = toPt.x + 1;
                    polyPts[5].x = fromPt.x + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = toPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = fromPt.y + brushSize.cy;
                    break;

                case 5:
                    polyPts[0].x = fromPt.x + brushSize.cy + 1;
                    polyPts[1].x = fromPt.x + brushSize.cy;
                    polyPts[2].x = fromPt.x;
                    polyPts[3].x = toPt.x;
                    polyPts[4].x = toPt.x + 1;
                    polyPts[5].x = toPt.x + brushSize.cy + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = fromPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = toPt.y;
                    break;

                default:
                    polyPts[0].x = fromPt.x + brushSize.cy;
                    polyPts[1].x = fromPt.x + brushSize.cy + 1;
                    polyPts[2].x = toPt.x + brushSize.cy + 1;
                    polyPts[3].x = toPt.x + 1;
                    polyPts[4].x = toPt.x;
                    polyPts[5].x = fromPt.x;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = toPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = fromPt.y + brushSize.cy;
                    break;
                }

            pDC->Polygon(polyPts, 6);
            }
            break;

        case slantedRightBrush:
            {
            CPoint polyPts [6];

            switch (octant)
                {
                case 0:
                case 1:
                    polyPts[0].x = fromPt.x + brushSize.cy;
                    polyPts[1].x = fromPt.x + brushSize.cy + 1;
                    polyPts[2].x = toPt.x + brushSize.cy + 1;
                    polyPts[3].x = toPt.x + 1;
                    polyPts[4].x = toPt.x;
                    polyPts[5].x = fromPt.x;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = toPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = fromPt.y;
                    break;

                case 2 :
                    polyPts[0].x = fromPt.x + brushSize.cy + 1;
                    polyPts[1].x = fromPt.x + brushSize.cy;
                    polyPts[2].x = fromPt.x;
                    polyPts[3].x = toPt.x;
                    polyPts[4].x = toPt.x + 1;
                    polyPts[5].x = toPt.x + brushSize.cy + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = fromPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = toPt.y + brushSize.cy;
                    break;

                case 3 :
                    polyPts[0].x = fromPt.x + brushSize.cy + 1;
                    polyPts[1].x = fromPt.x + brushSize.cy;
                    polyPts[2].x = toPt.x + brushSize.cy;
                    polyPts[3].x = toPt.x;
                    polyPts[4].x = toPt.x + 1;
                    polyPts[5].x = fromPt.x + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y + brushSize.cy;
                    polyPts[2].y = toPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y;
                    polyPts[5].y = fromPt.y;
                    break;

                case 4 :
                case 5 :
                    polyPts[0].x = fromPt.x + 1;
                    polyPts[1].x = fromPt.x;
                    polyPts[2].x = toPt.x;
                    polyPts[3].x = toPt.x + brushSize.cy;
                    polyPts[4].x = toPt.x + brushSize.cy + 1;
                    polyPts[5].x = fromPt.x + brushSize.cy + 1;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = toPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = fromPt.y + brushSize.cy;
                    break;

                case 6 :
                    polyPts[0].x = fromPt.x;
                    polyPts[1].x = fromPt.x + 1;
                    polyPts[2].x = fromPt.x + brushSize.cy + 1;
                    polyPts[3].x = toPt.x + brushSize.cy + 1;
                    polyPts[4].x = toPt.x + brushSize.cy;
                    polyPts[5].x = toPt.x;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = fromPt.y + brushSize.cy;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = toPt.y;
                    break;

                default :
                    polyPts[0].x = fromPt.x;
                    polyPts[1].x = fromPt.x + 1;
                    polyPts[2].x = toPt.x + 1;
                    polyPts[3].x = toPt.x + brushSize.cy + 1;
                    polyPts[4].x = toPt.x + brushSize.cy;
                    polyPts[5].x = fromPt.x + brushSize.cy;
                    polyPts[0].y = polyPts[1].y = fromPt.y;
                    polyPts[2].y = toPt.y;
                    polyPts[3].y = polyPts[4].y = toPt.y + brushSize.cy;
                    polyPts[5].y = fromPt.y + brushSize.cy;
                    break;
                }

            pDC->Polygon(polyPts, 6);
            }
            break;
        }

    pDC->SelectObject(pOldPen);
    }

/***************************************************************************/

void SetCombineMode(int wNewCombineMode)
    {
    wCombineMode = wNewCombineMode;
    }

/***************************************************************************/

static int      cxLastShape;
static int      cyLastShape;
static int      cxShapeBitmap;
static int      cyShapeBitmap;
static CBitmap  shapeBitmap;
static enum { ellipse, roundRect }  nLastShape;

void Mylipse(HDC hDC, int x1, int y1, int x2, int y2, BOOL bFilled)
    {
        COLORREF crNewBk, crNewText;
        GetMonoBltColors(hDC, NULL, crNewBk, crNewText);

    COLORREF crOldText = SetTextColor(hDC, crNewText);
    COLORREF crOldBk   = SetBkColor  (hDC, crNewBk);

    int cx = x2 - x1;
    int cy = y2 - y1;

    if (!bFilled)
        {
        Ellipse(hDC, x1, y1, x2, y2);
        }
    else
        if (cx == cy && cx > 0 && cx <= 8)
            {
            // HACK: The Windows Ellipse function sucks for small ellipses, so I
            // use some little bitmaps here for filled circles 1 to 8 pixels in
            // diameter.

            static CBitmap  g_ellipses [8];

            if (g_ellipses[cx - 1].m_hObject == NULL &&
                    ! g_ellipses[cx - 1].LoadBitmap(IDB_ELLIPSE1 + cx - 1))
                {
                theApp.SetMemoryEmergency();
                SetTextColor(hDC, crOldText);
                SetBkColor(hDC, crOldBk);
                return;
                }

            HDC hTempDC = CreateCompatibleDC(hDC);
            if (hTempDC == NULL)
                {
                theApp.SetGdiEmergency();
                SetTextColor(hDC, crOldText);
                SetBkColor(hDC, crOldBk);
                return;
                }

            HBITMAP hOldBitmap = (HBITMAP) SelectObject(hTempDC,
                    g_ellipses[cx - 1].m_hObject);
            BitBlt(hDC, x1, y1, cx, cy, hTempDC, 0, 0, DSPDxax);
            SelectObject(hTempDC, hOldBitmap);
            DeleteDC(hTempDC);
            }
        else
            if (cx > 0 && cy > 0)
                {
                // Actually, Ellipse() just plain sucks...  Let's do as much
                // as possible our selves to fix it!  Here we draw the ellipse
                // into a monochrome bitmap to get the shape and then use that
                // as a mask to get the current pattern into the imge.

                HDC hTempDC = CreateCompatibleDC(hDC);
                if (hTempDC == NULL)
                    {
                    theApp.SetGdiEmergency();
                    SetTextColor(hDC, crOldText);
                    SetBkColor(hDC, crOldBk);
                    return;
                    }

                BOOL bRefill = FALSE;

                if (cx > cxShapeBitmap || cy > cyShapeBitmap)
                    {
                    shapeBitmap.DeleteObject();
                    if (shapeBitmap.CreateBitmap(cx, cy, 1, 1, NULL))
                        {
                        cxShapeBitmap = cx;
                        cyShapeBitmap = cy;
                        bRefill = TRUE;
                        }
                    }

                if (shapeBitmap.m_hObject == NULL)
                    {
                    theApp.SetMemoryEmergency();
                    DeleteDC(hTempDC);
                    SetTextColor(hDC, crOldText);
                    SetBkColor(hDC, crOldBk);
                    return;
                    }

                if (cx != cxLastShape || cy != cyLastShape || nLastShape != ellipse)
                    {
                    cxLastShape = cx;
                    cyLastShape = cy;
                    nLastShape = ellipse;
                    bRefill = TRUE;
                    }

                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hTempDC,
                        shapeBitmap.m_hObject);

                if (bRefill)
                    {
                    PatBlt(hTempDC, 0, 0, cx, cy, BLACKNESS);
                    SelectObject(hTempDC, GetStockObject(WHITE_BRUSH));
                    SelectObject(hTempDC, GetStockObject(WHITE_PEN));
                    Ellipse(hTempDC, 0, 0, cx, cy);
                    }

                BitBlt(hDC, x1, y1, cx, cy, hTempDC, 0, 0, DSPDxax);

                SelectObject(hTempDC, hOldBitmap);
                DeleteDC(hTempDC);
                }

    SetTextColor(hDC, crOldText);
    SetBkColor(hDC, crOldBk);
    }

/***************************************************************************/
#ifdef XYZZYZ
void MyRoundRect(HDC hDC, int x1, int y1, int x2, int y2,
                 int nEllipseWidth, int nEllipseHeight, BOOL bFilled)
    {
    int cx = x2 - x1;
    int cy = y2 - y1;

    if (!bFilled)
        {
        RoundRect(hDC, x1, y1, x2, y2, nEllipseWidth, nEllipseHeight);
        return;
        }

    if (cx > 0 && cy > 0)
        {
        HDC hTempDC = CreateCompatibleDC(hDC);

        if (hTempDC == NULL)
            {
            theApp.SetGdiEmergency();
            return;
            }

        BOOL bRefill = FALSE;

        if (cx > cxShapeBitmap || cy > cyShapeBitmap)
            {
            shapeBitmap.DeleteObject();
            if (shapeBitmap.CreateBitmap(cx, cy, 1, 1, NULL))
                {
                cxShapeBitmap = cx;
                cyShapeBitmap = cy;
                bRefill = TRUE;
                }
            }

        if (shapeBitmap.m_hObject == NULL)
            {
            theApp.SetMemoryEmergency();
            DeleteDC(hTempDC);
            return;
            }

        if (cx != cxLastShape || cy != cyLastShape || nLastShape != roundRect)
            {
            cxLastShape = cx;
            cyLastShape = cy;
            nLastShape = roundRect;
            bRefill = TRUE;
            }

        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hTempDC,
                shapeBitmap.m_hObject);

        if (bRefill)
            {
            PatBlt(hTempDC, 0, 0, cx, cy, BLACKNESS);
            SelectObject(hTempDC, GetStockObject(WHITE_BRUSH));
            SelectObject(hTempDC, GetStockObject(WHITE_PEN));
            RoundRect(hTempDC, 0, 0, cx, cy, nEllipseWidth, nEllipseHeight);
            }

        BitBlt(hDC, x1, y1, cx, cy, hTempDC, 0, 0, DSna);
        BitBlt(hDC, x1, y1, cx, cy, hTempDC, 0, 0, DSPao);

        SelectObject(hTempDC, hOldBitmap);
        DeleteDC(hTempDC);
        }
    }
#endif
/***************************************************************************/

void PolyTo(CDC* pDC, CPoint fromPt, CPoint toPt, CSize size)
    {
    CPoint polyPts [6];

    if (toPt.x > fromPt.x)
        {
        polyPts[0].x = polyPts[1].x = fromPt.x;
        polyPts[2].x = toPt.x;
        polyPts[3].x = polyPts[4].x = toPt.x + size.cx;
        polyPts[5].x = fromPt.x + size.cx;
        }
    else
        {
        polyPts[0].x = polyPts[1].x = fromPt.x + size.cx;
        polyPts[2].x = toPt.x + size.cx;
        polyPts[3].x = polyPts[4].x = toPt.x;
        polyPts[5].x = fromPt.x;
        }

    if (toPt.y > fromPt.y)
        {
        polyPts[0].y = polyPts[5].y = fromPt.y;
        polyPts[1].y = fromPt.y + size.cy;
        polyPts[2].y = polyPts[3].y = toPt.y + size.cy;
        polyPts[4].y = toPt.y;
        }
    else
        {
        polyPts[0].y = polyPts[5].y = fromPt.y + size.cy;
        polyPts[1].y = fromPt.y;
        polyPts[2].y = polyPts[3].y = toPt.y;
        polyPts[4].y = toPt.y + size.cy;
        }

    if (pDC)
        pDC->Polygon(polyPts, 6);
    }

/***************************************************************************/

BOOL GetTanPt(CSize size, CPoint delta, CRect& tan)
    {
    int x, y;
    int xExt, yExt, theExt, xTemp;
    CDC dc;
    CBitmap* pOldBitmap, bitmap;

    size.cx += 1;
    size.cy += 1;

    tan.SetRect(0, 0, 0, 0);

    if (!dc.CreateCompatibleDC(NULL))
        {
        theApp.SetGdiEmergency();
        return FALSE;
        }

    if (!bitmap.CreateCompatibleBitmap(&dc, size.cx, size.cy))
        {
        theApp.SetMemoryEmergency();
        return FALSE;
        }

    VERIFY((pOldBitmap = dc.SelectObject(&bitmap)));

    TRY
        {
        CBrush cBrushWhite(PALETTERGB(0xff, 0xff, 0xff));
        CRect cRectTmp(0,0,size.cx, size.cy);
        dc.FillRect(&cRectTmp, &cBrushWhite);
//    dc.PatBlt(0, 0, size.cx, size.cy, WHITENESS);
        }
    CATCH(CResourceException, e)
        {
        }
    END_CATCH
    dc.SelectStockObject(NULL_PEN);
    dc.SelectStockObject(BLACK_BRUSH);
    Mylipse(dc.m_hDC, 0, 0, size.cx, size.cy, TRUE);

    yExt = 0;
    for (xExt = 0; xExt < size.cx - 1; xExt++)
        {
        if (dc.GetPixel(xExt, 0) == 0)
            break;
        }
    theExt = 10 * xExt;

    if (delta.y == 0)
        {
        tan.SetRect(xExt, 0, xExt, size.cy - 1);
        }
    else
        {
        for (y = 0; y < size.cy; y++)
            {
            for (x = 0; x < size.cx - 1; x++)
                {
                if (dc.GetPixel(x, y) == 0)
                    break;
                }

            xTemp = 10 * x - 10 * y * delta.x / delta.y;
            if (theExt > xTemp)
                {
                xExt = x;
                yExt = y;
                theExt = xTemp;
                }
            else
                if (theExt < xTemp)
                    {
                    break;
                    }
            }

        tan.left = xExt;
        tan.top = yExt;

        for (y = 0; y < size.cy; y++)
            {
            for (x = size.cx - 1; x > 0; x--)
                {
                if (dc.GetPixel(x, y) == 0)
                    break;
                }
            xTemp = 10 * x - 10 * y * delta.x / delta.y;
            if (theExt < xTemp)
                {
                xExt = x;
                yExt = y;
                theExt = xTemp;
                }
            else
                if (theExt > xTemp)
                    {
                    break;
                    }
            }

        tan.right = xExt;
        tan.bottom = yExt;
        }

    dc.SelectObject(pOldBitmap);

    return TRUE;
    }

/***************************************************************************/

void PickupSelection()
    {
    // Time to pick up the bits!
    ASSERT(theImgBrush.m_pImg);

    if (theImgBrush.m_pImg == NULL)
        return;

    CPalette* ppalOld = theImgBrush.SetBrushPalette( &theImgBrush.m_dc, FALSE ); // Background ??
    CDC* pImgDC = CDC::FromHandle( theImgBrush.m_pImg->hDC );

    if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        TRY {
            CBrush cBrushBk( crRight );
            CRect  cRectTmp( 0, 0, theImgBrush.m_size.cx,
                                   theImgBrush.m_size.cy );

            theImgBrush.m_dc.FillRect( &cRectTmp, &cBrushBk );
            }
        CATCH(CResourceException, e)
            {
            theApp.SetGdiEmergency();
            return;
            }
        END_CATCH

        if (theImgBrush.m_cRgnPolyFreeHandSel.GetSafeHandle())
            {
            theImgBrush.m_dc.SelectClipRgn(&theImgBrush.m_cRgnPolyFreeHandSel);

            theImgBrush.m_dc.StretchBlt( 0, 0, theImgBrush.m_size.cx,
                                               theImgBrush.m_size.cy,
                                       pImgDC, theImgBrush.m_rcSelection.left,
                                               theImgBrush.m_rcSelection.top,
                                               theImgBrush.m_rcSelection.Width(),
                                               theImgBrush.m_rcSelection.Height(),
                                               SRCCOPY );

            theImgBrush.m_dc.SelectClipRgn(NULL);
            }
        }
    else
        {
        theImgBrush.m_dc.BitBlt( 0, 0, theImgBrush.m_size.cx,
                                       theImgBrush.m_size.cy,
                               pImgDC, theImgBrush.m_rcSelection.left,
                                       theImgBrush.m_rcSelection.top, SRCCOPY );
        }

    if (ppalOld)
        theImgBrush.m_dc.SelectPalette( ppalOld, FALSE );

    theImgBrush.RecalcMask( crRight );
    }

/***************************************************************************/

void CommitSelection(BOOL bSetUndo)
    {
    if (theImgBrush.m_bMakingSelection)
        return;

    if (! theImgBrush.m_bSmearSel
    &&  ( theImgBrush.m_pImg == NULL
       || theImgBrush.m_bFirstDrag
       || theImgBrush.m_bLastDragWasASmear))
        {
        return;
        }

    if (theImgBrush.m_bLastDragWasFirst
    && (theImgBrush.m_bMoveSel
     || theImgBrush.m_bSmearSel))
        {
        return;
        }

    TRACE1( "CommitSelection(%d)\n", bSetUndo );

    if (bSetUndo)
        {
        HideBrush();
        CRect rectUndo = theImgBrush.m_rcSelection;

        if (theImgBrush.m_bLastDragWasFirst)
            {
            theImgBrush.m_bLastDragWasFirst = FALSE;
            rectUndo |= theImgBrush.m_rcDraggedFrom;
            }
        else
            {
            SetUndo(theImgBrush.m_pImg);
            }

        CImgWnd::c_pImgWndCur->FinishUndo(rectUndo);
        }

    if (CImgTool::GetCurrentID() != IDMX_TEXTTOOL)
        {
        if (theImgBrush.m_bOpaque)
            {
            theImgBrush.BltReplace( theImgBrush.m_pImg,
                                    theImgBrush.m_rcSelection.TopLeft() );
            }
        else
            {
            theImgBrush.BltMatte( theImgBrush.m_pImg,
                                  theImgBrush.m_rcSelection.TopLeft() );
            }
        }

    InvalImgRect ( theImgBrush.m_pImg, &theImgBrush.m_rcSelection );
    CommitImgRect( theImgBrush.m_pImg, &theImgBrush.m_rcSelection );

    DirtyImg( theImgBrush.m_pImg );
    }

/***************************************************************************/

void AddImgWnd(IMG* pimg, CImgWnd* pImgWnd)
    {
    ASSERT(pImgWnd->m_pNextImgWnd == NULL);
    pImgWnd->m_pNextImgWnd = pimg->m_pFirstImgWnd;
    pimg->m_pFirstImgWnd = pImgWnd;
    }

/***************************************************************************/

void GetImgSize(IMG* pImg, CSize& size)
    {
    size.cx = pImg->cxWidth;
    size.cy = pImg->cyHeight;
    }

/***************************************************************************/

BOOL SetImgSize(IMG* pImg, CSize newSize, BOOL bStretch)
    {
    if (newSize.cx != pImg->cxWidth
    ||  newSize.cy != pImg->cyHeight)
        {
        HBITMAP hNewBitmap = CreateCompatibleBitmap( pImg->hDC,
                                                     newSize.cx, newSize.cy );
        if (hNewBitmap == NULL)
            {
            return FALSE;
            }
        HDC hDC = CreateCompatibleDC( pImg->hDC );

        if (hDC == NULL)
            {
            DeleteObject( hNewBitmap );
            return FALSE;
            }
        HBITMAP hOldBitmap = (HBITMAP)SelectObject( hDC, hNewBitmap );

        ASSERT( hOldBitmap );

        HPALETTE hpalOld = NULL;

        if (theApp.m_pPalette
        &&  theApp.m_pPalette->m_hObject)
            {
            hpalOld = SelectPalette( hDC, (HPALETTE)theApp.m_pPalette->m_hObject, FALSE ); // Background ??
            RealizePalette( hDC );
            }

        if (bStretch)
            {
            UINT uStretch = HALFTONE;


            if (pImg->cPlanes * pImg->cBitCount == 1)
                {
                uStretch = BLACKONWHITE;

                if (GetRValue( crLeft )
                ||  GetGValue( crLeft )
                ||  GetBValue( crLeft ))
                    uStretch = WHITEONBLACK;
                }

            SetStretchBltMode( hDC, uStretch );

            StretchCopy( hDC, 0, 0, newSize.cx, newSize.cy,
                   pImg->hDC, 0, 0, pImg->cxWidth, pImg->cyHeight );
            }
        else
            {
            // Fill it with the background color first!
            HBRUSH hBrush  = ::CreateSolidBrush(crRight);

            ASSERT(hBrush);

            HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);

            ASSERT(hOldBrush);

            PatBlt(hDC, 0, 0, newSize.cx, newSize.cy, PATCOPY);

            VERIFY(SelectObject(hDC, hOldBrush) == hBrush);

            DeleteObject(hBrush);

            BitBlt(hDC, 0, 0, newSize.cx, newSize.cy, pImg->hDC, 0, 0, SRCCOPY);
            }

        if (hpalOld)
            SelectPalette( hDC, hpalOld, FALSE ); // Background ??

        VERIFY( SelectObject(       hDC, hOldBitmap) ==    hNewBitmap );
        VERIFY( SelectObject( pImg->hDC, hNewBitmap) == pImg->hBitmap );

        DeleteObject( pImg->hBitmap );
        DeleteDC( hDC );

        pImg->hBitmap  = hNewBitmap;
        pImg->cxWidth  = newSize.cx;
        pImg->cyHeight = newSize.cy;

        pRubberImg = NULL;

        SetupRubber( pImg );

        InvalImgRect(pImg, NULL);

        CImgWnd* pImgWnd = pImg->m_pFirstImgWnd;

        while (pImgWnd)
            {
            pImgWnd->Invalidate( FALSE );
            pImgWnd->CheckScrollBars();

            pImgWnd = pImgWnd->m_pNextImgWnd;
            }
        }

    return TRUE;
    }

/***************************************************************************/

// This may be set to TRUE via the "DriverCanStretch" entry in the INI file.
// Doing so will speed up the graphics editor, but will cause some device
// drivers to crash (hence the FALSE default).
//
BOOL  g_bDriverCanStretch = TRUE;

void StretchCopy( HDC hdcDest, int xDest, int yDest, int cxDest, int cyDest,
                  HDC hdcSrc , int xSrc , int ySrc , int cxSrc , int cySrc )
    {
    if (cxDest == cxSrc && cyDest == cySrc)
        {
        // No point in using the trick if we're not really stretching...

        BitBlt(hdcDest, xDest, yDest, cxDest, cyDest,
               hdcSrc,  xSrc,  ySrc, SRCCOPY);
        }
    else
        if (g_bDriverCanStretch ||
            cxDest < 0 || cyDest < 0 || cxSrc < 0 || cySrc < 0)
            {
            // We can't use the following trick when flipping, but the
            // driver will usually pass things on to GDI here anyway...

            StretchBlt(hdcDest, xDest, yDest, cxDest, cyDest,
                       hdcSrc, xSrc, ySrc, cxSrc, cySrc, SRCCOPY);
            }
        else
            {
            // Some drivers (e.g. Paradise) crash on memory to memory stretches,
            // so we trick them into just using GDI here by not using SRCCOPY...

            PatBlt(hdcDest, xDest, yDest, cxDest, cyDest, BLACKNESS);

            StretchBlt(hdcDest, xDest, yDest, cxDest, cyDest,
                        hdcSrc, xSrc, ySrc, cxSrc, cySrc, DSo);
            }
    }

/***************************************************************************/

void StandardiseCoords(CPoint* s, CPoint* e)
    {
    if (s->x > e->x)
        {
        int tx;

        tx = s->x;
        s->x = e->x;
        e->x = tx;
        }

    if (s->y > e->y)
        {
        int ty;

        ty = s->y;
        s->y = e->y;
        e->y = ty;
        }
    }

/***************************************************************************/

CPalette* MergePalettes( CPalette *pPal1, CPalette *pPal2, int& iAdds )
    {
    int           iPal1NumEntries;
    int           iPal2NumEntries;
    LOGPALETTE256 LogPal1;
    LOGPALETTE256 LogPal2;
    CPalette*     pPalMerged = NULL;

    iAdds = 0;

    if (pPal1 == NULL ||  pPal2 == NULL)
        return NULL;

    iPal1NumEntries = pPal1->GetPaletteEntries( 0, 256, &LogPal1.palPalEntry[0] );
    pPal1->GetPaletteEntries( 0, iPal1NumEntries, &LogPal1.palPalEntry[0] );

    iPal2NumEntries = pPal2->GetPaletteEntries( 0, 256, &LogPal2.palPalEntry[0] );
    pPal2->GetPaletteEntries( 0, iPal2NumEntries, &LogPal2.palPalEntry[0] );

    // check if room left in 1st palette to merge.  If no room, then use 1st palette
    for (int i = 0; i < iPal2NumEntries
                     && iPal1NumEntries < MAX_PALETTE_COLORS; i++)
        {
        for (int j = 0; j < iPal1NumEntries; j++)
            {
            if (LogPal1.palPalEntry[j].peRed   == LogPal2.palPalEntry[i].peRed
            &&  LogPal1.palPalEntry[j].peGreen == LogPal2.palPalEntry[i].peGreen
            &&  LogPal1.palPalEntry[j].peBlue  == LogPal2.palPalEntry[i].peBlue)
                break;
            }

        if (j < iPal1NumEntries)
            continue;  // found one

        // color was not found in 1st palette add it if room
        LogPal1.palPalEntry[iPal1NumEntries++] = LogPal2.palPalEntry[i];
        iAdds++;
        }

    LogPal1.palVersion    = 0x300;
    LogPal1.palNumEntries = (WORD)iPal1NumEntries;

    pPalMerged = new CPalette();

    if (pPalMerged)
        if (! CreateSafePalette(pPalMerged, (LPLOGPALETTE)&LogPal1 ))
            {
            delete pPalMerged;
            pPalMerged = NULL;
            }

    return pPalMerged;
    }

/******************************************************************************/

void AdjustPointForGrid(CPoint *ptPointLocation)
    {
    if (theApp.m_iSnapToGrid != 0)
        {
        int iNextGridOffset;

        iNextGridOffset = ptPointLocation->x % theApp.m_iGridExtent;
        // if distance to next grid is less than 1/2 distance between grids
        // closer to previous grid
        if (iNextGridOffset <= theApp.m_iGridExtent/2)
            {
            iNextGridOffset *= -1; // closer to previous grid location
            }
        else
            {
            iNextGridOffset = theApp.m_iGridExtent -iNextGridOffset; // closer to next grid location
            }
        ptPointLocation->x = ptPointLocation->x + iNextGridOffset;

        // if distance to next grid is less than 1/2 distance between grids
        // closer to previous grid
        iNextGridOffset = ptPointLocation->y % theApp.m_iGridExtent;
        if (iNextGridOffset <= theApp.m_iGridExtent/2)
            {
            iNextGridOffset *= -1; // closer to previous grid location
            }
        else
            {
            iNextGridOffset = theApp.m_iGridExtent -iNextGridOffset; // closer to next grid location
            }

        ptPointLocation->y = ptPointLocation->y + iNextGridOffset;
        }
    }

static unsigned short  bmapHorzBorder [] =
                                { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 };

static unsigned short  bmapVertBorder [] =
                                { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 };

       CBrush   g_brSelectHorz;
       CBrush   g_brSelectVert;
static CBitmap  m_bmSelectHorz;
static CBitmap  m_bmSelectVert;

void InitCustomData()                           // called once in main
    {
    //
    // no windows critical error message box, return errors to open call
    //
    if (m_bmSelectHorz.CreateBitmap( 8, 8, 1, 1, (LPSTR)bmapHorzBorder ))
        g_brSelectHorz.CreatePatternBrush( &m_bmSelectHorz );

    if (m_bmSelectVert.CreateBitmap( 8, 8, 1, 1, (LPSTR)bmapVertBorder ))
        g_brSelectVert.CreatePatternBrush( &m_bmSelectVert );

    SetErrorMode( SEM_FAILCRITICALERRORS );
    }

//
// make sure global resoures are freed
//
void CustomExit()
    {
    if (g_brSelectHorz.m_hObject)
        g_brSelectHorz.DeleteObject();

    if (m_bmSelectHorz.m_hObject)
        m_bmSelectHorz.DeleteObject();

    if (g_brSelectVert.m_hObject)
        g_brSelectVert.DeleteObject();

    if (m_bmSelectVert.m_hObject)
        m_bmSelectVert.DeleteObject();
    }

/*
int FileTypeFromExtension( const TCHAR far* lpcExt )
    {
    if (*lpcExt == TEXT('.'))        // skip the . in .*
        lpcExt++;

    // must be redone
    return NULL;
    }
*/

CPalette *PBSelectPalette(CDC *pDC, CPalette *pPalette, BOOL bForceBk)
{
        if (!pPalette)
        {
                return(NULL);
        }

        if (IsInPlace())
        {
                bForceBk = TRUE;
        }

        CPalette *ppalOld = pDC->SelectPalette( theApp.m_pPalette, bForceBk );
        pDC->RealizePalette();

        return(ppalOld);
}

