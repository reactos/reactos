// printres.cpp : implementation of the CPrintResObj class
//
// #define PAGESETUP

#include "stdafx.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "pbrusdoc.h"
#include "imgwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "printres.h"
#include "cmpmsg.h"
#include "imageatt.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CPrintResObj, CObject )

#include "memtrace.h"

// TIN = thousandths of inches
// HMM = hundredths of millimeters
#define TIN_TO_PIXEL(nLen, nPpi)  ((nLen * nPpi + 500) / 1000)
#define HMM_TO_PIXEL(nLen, nPpdm) ((nLen * nPpdm + 5000) / 10000)
#define PIXEL_TO_TIN(nLen, nPpi)  ((nLen * 1000) / nPpi)
#define PIXEL_TO_HMM(nLen, nPpdm) ((nLen * 10000) / nPpdm)
#define TIN_TO_HMM(nLen) (((nLen * 254) + 50) / 100)
#define HMM_TO_TIN(nLen) (((nLen * 3937) + 5000) / 10000)


void MulDivRect(LPRECT r1, LPRECT r2, int num, int div)
{
        r1->left = MulDiv(r2->left, num, div);
        r1->top = MulDiv(r2->top, num, div);
        r1->right = MulDiv(r2->right, num, div);
        r1->bottom = MulDiv(r2->bottom, num, div);
}


/***************************************************************************/
// CPrintResObj implementation

CPrintResObj::CPrintResObj( CPBView* pView, CPrintInfo* pInfo )
    {
    m_pDIB        = NULL;
    m_pDIBpalette = NULL;

    if (pInfo                                  == NULL
    ||  pView                                  == NULL
    ||  pView->m_pImgWnd                       == NULL
    ||  pView->m_pImgWnd->m_pImg               == NULL
    ||  pView->m_pImgWnd->m_pImg->m_pBitmapObj == NULL)
        return;

    m_pView = pView;

    m_dwPicWidth  = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_nWidth;
    m_dwPicHeight = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_nHeight;

    BOOL bOldFlag = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty;

    //  force the resource to save itself then use the dib to print
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty = TRUE;
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->SaveResource( TRUE );
    m_pDIB = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_lpvThing;
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty = bOldFlag;

    if (m_pDIB == NULL)
        return;

    m_pDIBpalette = CreateDIBPalette( (LPSTR)m_pDIB );
    m_pDIBits     = FindDIBBits     ( (LPSTR)m_pDIB );

    // save the scroll value off, then set to 0,0
    m_cSizeScroll = m_pView->m_pImgWnd->GetScrollPos();

    // save the zoom value off, then set to 100%
    m_iZoom      = m_pView->m_pImgWnd->GetZoom();
    m_iPagesWide = 1;
    m_iPagesHigh = 1;
    m_rtMargins.SetRectEmpty();

    pInfo->m_nNumPreviewPages = 1;
    pInfo->m_lpUserData       = this;
    }

/***************************************************************************/

void CPrintResObj::BeginPrinting( CDC* pDC, CPrintInfo* pInfo )
{
    if (pDC               == NULL
    ||  pDC->GetSafeHdc() == NULL)
        return;

    int iWidthinMM    = pDC->GetDeviceCaps( HORZSIZE );
    int iHeightinMM   = pDC->GetDeviceCaps( VERTSIZE );

    m_iWidthinPels  = pDC->GetDeviceCaps( HORZRES );
    m_iHeightinPels = pDC->GetDeviceCaps( VERTRES );

    int ixPelsPerDM   = (int)(((((long)m_iWidthinPels  * 1000L) / (long)iWidthinMM ) + 5L) / 10);
    int iyPelsPerDM   = (int)(((((long)m_iHeightinPels * 1000L) / (long)iHeightinMM) + 5L) / 10);

    m_pView->m_pImgWnd->SetScroll( 0, 0 );
    m_pView->m_pImgWnd->SetZoom  ( 1 );

    // convert the screen to printer coordinates
    // this will give us the 'physical size' of the bitmap in printer units
    m_dwPrtWidth  = (m_dwPicWidth  * 1000 / theApp.ScreenDeviceInfo.ixPelsPerDM * ixPelsPerDM + 500) / 1000;
    m_dwPrtHeight = (m_dwPicHeight * 1000 / theApp.ScreenDeviceInfo.iyPelsPerDM * iyPelsPerDM + 500) / 1000;

    m_rtMargins.left   = HMM_TO_PIXEL( theApp.m_rectMargins.left  , ixPelsPerDM );
    m_rtMargins.top    = HMM_TO_PIXEL( theApp.m_rectMargins.top   , iyPelsPerDM );
    m_rtMargins.right  = HMM_TO_PIXEL( theApp.m_rectMargins.right , ixPelsPerDM );
    m_rtMargins.bottom = HMM_TO_PIXEL( theApp.m_rectMargins.bottom, iyPelsPerDM );

        CRect rcMinMargins;

        int nPageWidth = pDC->GetDeviceCaps(PHYSICALWIDTH);
        int nPageHeight = pDC->GetDeviceCaps(PHYSICALHEIGHT);

        int nOffsetWidth = pDC->GetDeviceCaps(PHYSICALOFFSETX);
        int nOffsetHeight = pDC->GetDeviceCaps(PHYSICALOFFSETY);

        // calculate min margins in PIXELS;
        rcMinMargins.left   = nOffsetWidth;
        rcMinMargins.top    = nOffsetHeight;
        rcMinMargins.right  = nPageWidth - m_iWidthinPels - nOffsetWidth;
        rcMinMargins.bottom = nPageHeight - m_iHeightinPels - nOffsetHeight;

        m_rtMargins.left   -= rcMinMargins.left;
        m_rtMargins.top    -= rcMinMargins.top ;
        m_rtMargins.right  -= rcMinMargins.right;
        m_rtMargins.bottom -= rcMinMargins.bottom;

        m_rtMargins.left   = max(0, m_rtMargins.left  );
        m_rtMargins.top    = max(0, m_rtMargins.top   );
        m_rtMargins.right  = max(0, m_rtMargins.right );
        m_rtMargins.bottom = max(0, m_rtMargins.bottom);

        // Quick sanity check
        if (m_rtMargins.left + m_rtMargins.right >= m_iWidthinPels)
        {
                m_rtMargins.left = m_rtMargins.right = 0;
        }
        if (m_rtMargins.top + m_rtMargins.bottom >= m_iHeightinPels)
        {
                m_rtMargins.top = m_rtMargins.bottom = 0;
        }

    // adjust the print area for the margins
    m_iWidthinPels  -= (m_rtMargins.left + m_rtMargins.right );
    m_iHeightinPels -= (m_rtMargins.top  + m_rtMargins.bottom);

    iWidthinMM  = m_iWidthinPels  * 100 / ixPelsPerDM;
    iHeightinMM = m_iHeightinPels * 100 / iyPelsPerDM;

    // we need the 'page' size of the bitmap - 'size of page in mm * screen pels per mm'
    // but this is too crude, the rounding errors are to great so we will do the
    // equivalent with other numbers.
    m_iPageWidthinScreenPels  = (int)(((LONG)iWidthinMM  * (LONG)theApp.ScreenDeviceInfo.ixPelsPerDM + 50L) / 100L);
    m_iPageHeightinScreenPels = (int)(((LONG)iHeightinMM * (LONG)theApp.ScreenDeviceInfo.iyPelsPerDM + 50L) / 100L);

    // if the image is wider or longer than a page of the printer, calculate
    // how many pages down and across will be needed to print the whole image.
    m_iPagesWide = (int)((m_dwPicWidth  + m_iPageWidthinScreenPels  - 1) / m_iPageWidthinScreenPels );
    m_iPagesHigh = (int)((m_dwPicHeight + m_iPageHeightinScreenPels - 1) / m_iPageHeightinScreenPels);

    pInfo->SetMaxPage( m_iPagesWide * m_iPagesHigh );

    // If only printing 1 page, should not be in 2 page mode
    if (m_iPagesWide * m_iPagesHigh == 1)
    {
        pInfo->m_nNumPreviewPages = 1;
    }
}

/******************************************************************************/
/* We not only move the window origin to allow us to print multiple pages      */
/* wide but we also scale both the viewport and window extents to make them   */
/* proportional (i.e. a line on the screen is the same size as on             */
/* the printer). The pages to print are numbered across.  For      +---+---+  */
/* example, if there  were 4 pages to print, then the first row    | 1 | 2 |  */
/* would have pages 1,2 and the second row would  have pages 3,4.  +---+---+  */
/*                                                                 | 3 | 4 |  */
/*                                                                 +---+---+  */
/*                                                                            */
/******************************************************************************/

void CPrintResObj::PrepareDC( CDC* pDC, CPrintInfo* pInfo )
    {
    if (pDC == NULL || pInfo == NULL)
        return;

    pDC->SetMapMode( MM_TEXT );
    pDC->SetStretchBltMode( HALFTONE );
    }

/***************************************************************************/

BOOL CPrintResObj::PrintPage( CDC* pDC, CPrintInfo* pInfo )
    {

    CPalette* ppalOld = NULL;

    if (m_pDIB == NULL)
        return FALSE;

    if (m_pDIBpalette != NULL)
        {
        ppalOld = pDC->SelectPalette( m_pDIBpalette, FALSE );
        pDC->RealizePalette();
        }


    // Calculate the row and column numbers
    int iPageCol = (pInfo->m_nCurPage - 1) % m_iPagesWide;
    int iPageRow = (pInfo->m_nCurPage - 1) / m_iPagesWide;

    int ixSrc   = iPageCol * m_iPageWidthinScreenPels;
    int iySrc   = iPageRow * m_iPageHeightinScreenPels;
    int icxSrc  = m_iPageWidthinScreenPels;
    int icySrc  = m_iPageHeightinScreenPels;
    int iRemX   = (int)((LONG)m_dwPicWidth  - ixSrc);
    int iRemY   = (int)((LONG)m_dwPicHeight - iySrc);

    icxSrc = min( icxSrc, iRemX );
    icySrc = min( icySrc, iRemY );

    // Always calculate dest sizes based on src sizes to avoid strange
    // stretching on the right side and bottom
    int icxDest = icxSrc * m_iWidthinPels  / m_iPageWidthinScreenPels;
    int icyDest = icySrc * m_iHeightinPels / m_iPageHeightinScreenPels;

    // DIB's are upside down
    iySrc   = m_dwPicHeight - iySrc - icySrc;

    if (GDI_ERROR == StretchDIBits( pDC->m_hDC, m_rtMargins.left,
                               m_rtMargins.top,
                               icxDest, icyDest,
                               ixSrc  , iySrc,
                               icxSrc , icySrc, m_pDIBits,
                   (LPBITMAPINFO)m_pDIB, DIB_RGB_COLORS, SRCCOPY ))
        {
        CmpMessageBox( IDS_ERROR_PRINTING, AFX_IDS_APP_TITLE, MB_OK | MB_ICONEXCLAMATION );
        }

    if (ppalOld != NULL)
        pDC->SelectPalette( ppalOld, FALSE );

    return TRUE;
    }

/***************************************************************************/

void CPrintResObj::EndPrinting( CDC* pDC, CPrintInfo* pInfo )
    {
    if (pDC != NULL)
        {
        m_pView->m_pImgWnd->SetScroll( m_cSizeScroll.cx, m_cSizeScroll.cy );

        // restore the zoom value
        m_pView->m_pImgWnd->SetZoom( m_iZoom );
        }

    if (m_pDIBpalette != NULL)
        delete m_pDIBpalette;

    delete this;
    }

/***************************************************************************/

inline int roundleast(int n)
{
        int mod = n%10;
        n -= mod;
        if (mod >= 5)
                n += 10;
        else if (mod <= -5)
                n -= 10;
        return n;
}

static void RoundRect(LPRECT r1)
{
        r1->left = roundleast(r1->left);
        r1->right = roundleast(r1->right);
        r1->top = roundleast(r1->top);
        r1->bottom = roundleast(r1->bottom);
}


void CPBView::OnFilePageSetup()
{
    CPageSetupDialog dlg;
    PAGESETUPDLG& psd = dlg.m_psd;
    TCHAR szMetric[2];
    BOOL bMetric;
    LCID lcidThread;
    //
    // We should use metric if the user has chosen CM in the
    // Image Attributes dialog, OR if using Pels and the NLS
    // setting is for metric
    //
    if (theApp.m_iCurrentUnits == ePIXELS)
    {
       lcidThread = GetThreadLocale();
       GetLocaleInfo (lcidThread, LOCALE_IMEASURE, szMetric, 2);
       bMetric = (szMetric[0] == TEXT('0'));
    }
    else
    {
       bMetric = ((eUNITS)theApp.m_iCurrentUnits == eCM); //centimeters
    }

    psd.Flags |= PSD_MARGINS | (bMetric ? PSD_INHUNDREDTHSOFMILLIMETERS :
        PSD_INTHOUSANDTHSOFINCHES);
    int nUnitsPerInch = bMetric ? 2540 : 1000;
    MulDivRect(&psd.rtMargin, theApp.m_rectMargins, nUnitsPerInch, MARGINS_UNITS);
    RoundRect(&psd.rtMargin);
// get the current device from the app
    PRINTDLG pd;
    pd.hDevNames = NULL;
    pd.hDevMode = NULL;
    theApp.GetPrinterDeviceDefaults(&pd);
    psd.hDevNames = pd.hDevNames;
    psd.hDevMode = pd.hDevMode;

    if (dlg.DoModal() == IDOK)
    {
        RoundRect(&psd.rtMargin);
        MulDivRect(theApp.m_rectMargins, &psd.rtMargin, MARGINS_UNITS, nUnitsPerInch);
        //theApp.m_rectPageMargin = m_rectMargin;
        theApp.SelectPrinter(psd.hDevNames, psd.hDevMode);

    }

    // PageSetupDlg failed
//    if (CommDlgExtendedError() != 0)
//    {
       //
       // bugbug - nothing to handle this failure
       //
//    }
}

