// pictures.cpp : This is the code for the picture object
//

#include "stdafx.h"
#include "resource.h"
#include "pictures.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CPic, CDC )

#include "memtrace.h"

/****************************************************************************/

CPic::CPic()
     : CDC()
{
mhBitmapOld = NULL;
mbReady     = FALSE;
/*
**  set up our DC
*/
if (! CreateCompatibleDC( NULL ))
    {
    #ifdef _DEBUG
    OutputDebugString( TEXT("GDI error or unable to get a DC!\r\n") );
    #endif
    }
}

/****************************************************************************/

CPic::~CPic()
{
if (m_hDC)
    {
    if (mhBitmapOld)
        SelectObject( CBitmap::FromHandle( mhBitmapOld ) );

    if (mBitmap.m_hObject)
        mBitmap.DeleteObject();

    if (mMask.m_hObject)
        mMask.DeleteObject();
    }
}

/****************************************************************************/

void CPic::Picture( CDC* pDC, int iX, int iY, int iPic )
{
if (! mbReady || iPic < 0 || iPic >= miCnt)
    return;

int iPicX = iPic * mSize.cx;

SelectObject( &mMask );

// select  FG color to be Black and BK color to be White
//
// The Default Mono->Color Conversion sets (Black -> FG Color, White -> BG Color)
// It uses FG/BK color from the destination (color DC).
// we want Black -> black, White -> white
// a black/white bitmap in color format.
COLORREF cRefFGColorOld = pDC->SetTextColor( RGB(0,0,0) );
COLORREF cRefBKColorOld = pDC->SetBkColor(RGB(255,255,255));

pDC->BitBlt( iX, iY, mSize.cx, mSize.cy, this, iPicX, 0, SRCAND );

pDC->SetTextColor(cRefFGColorOld);
pDC->SetBkColor(cRefBKColorOld);


SelectObject( &mBitmap );

pDC->BitBlt( iX, iY, mSize.cx, mSize.cy, this, iPicX, 0, SRCPAINT );
}

/****************************************************************************/

BOOL CPic::PictureSet( LPCTSTR lpszResourceName, int iCnt )
{
BOOL bReturn = FALSE;
/*
**  get the Pictures bitmap
*/
if (m_hDC && iCnt)
    if (mBitmap.LoadBitmap( lpszResourceName ))
        {
        miCnt = iCnt;

        bReturn = InstallPicture();
        }
    else
        {
        #ifdef _DEBUG
        OutputDebugString( TEXT("Unable to load the bitmap!\r\n") );
        #endif
        }

return bReturn;
}

/****************************************************************************/

BOOL CPic::PictureSet( UINT nIDResource, int iCnt )
{
BOOL bReturn = FALSE;
/*
**  get the Pictures bitmap
*/
if (m_hDC && iCnt)
    if (mBitmap.LoadBitmap( nIDResource ))
        {
        miCnt = iCnt;

        bReturn = InstallPicture();
        }
    else
        {
        #ifdef _DEBUG
        OutputDebugString( TEXT("Unable to load the bitmap!\r\n") );
        #endif
        }
return bReturn;
}

/****************************************************************************/

BOOL CPic::InstallPicture()
{
/*
**  get the bitmap info from the picture bitmap, saving the picture size
*/
BITMAP bmInfo;

if (mBitmap.GetObject( sizeof( BITMAP ), &bmInfo ) != sizeof( BITMAP ))
    {
    #ifdef _DEBUG
    OutputDebugString( TEXT("GDI error getting bitmap information!\r\n") );
    #endif

    return FALSE;
    }

mSize = CSize( bmInfo.bmWidth / miCnt, bmInfo.bmHeight );
/*
**  put the bitmap in the DC, saving the original.
*/
CBitmap* bitmap = SelectObject( &mBitmap );

mhBitmapOld = (HBITMAP)bitmap->m_hObject;
/*
**  create the mask bitmap, same size monochrome
*/
if (! mMask.CreateBitmap( bmInfo.bmWidth, bmInfo.bmHeight, 1, 1, NULL ))
    {
    #ifdef _DEBUG
    OutputDebugString( TEXT("GDI error creating the mask bitmap!\r\n") );
    #endif

    return FALSE;
    }
/*
**  put the mask in a temp DC so we can generate the mask bits
*/
CDC dc;

dc.CreateCompatibleDC( this );

ASSERT( dc.m_hDC );

CBitmap* ob = dc.SelectObject( &mMask );
/*
**  use the color at the upper left corner for generating the mask
*/
SetBkColor( GetPixel( 1, 1 ) );

// this ROP Code will leave bits in the destination bitmap the same color if the
// corresponding source bitmap's bit are black.
// all other bits in the destination (where source bits are not black)
// are turned to black.

#define ROP_DSna 0x00220326L
/*
**  Creates the mask from all pixels in the image of a given color.
**  Copies to the mask, then cuts the image with the mask.
*/
// create the mast, All but the background color is Black
// bkcolor is white
dc.BitBlt( 0, 0, bmInfo.bmWidth, bmInfo.bmHeight, this, 0, 0, SRCCOPY  );

// select  FG color to be Black and BK color to be White
// The Default Mono->Color Conversion sets (Black -> FG Color, White -> BG Color)
// It uses FG/BK color from the destination (color DC).
// we want Black -> black, White -> white
// a black/white bitmap in color format.
COLORREF cRefFGColorOld = dc.SetTextColor( RGB(0,0,0) );
COLORREF cRefBKColorOld = dc.SetBkColor(RGB(255,255,255));

   BitBlt( 0, 0, bmInfo.bmWidth, bmInfo.bmHeight,  &dc, 0, 0, ROP_DSna );

dc.SetTextColor(cRefFGColorOld);
dc.SetBkColor(cRefBKColorOld);

dc.SelectObject( ob );
mbReady = TRUE;
return TRUE;
}

/****************************************************************************/
