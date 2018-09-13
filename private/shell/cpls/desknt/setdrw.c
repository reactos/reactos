#include <windows.h>
#include "settings.h"


/****************************************************************************

    FUNCTION: MakeRect 

    PURPOSE:  Fill in RECT structure given contents. 

****************************************************************************/
VOID MakeRect( PRECT pRect, INT xmin, INT ymin, INT xmax, INT ymax )
{
    pRect->left= xmin;
    pRect->right= xmax;
    pRect->bottom= ymin;
    pRect->top= ymax;
}

// type constants for DrawArrow

#define AW_TOP      1   // top
#define AW_BOTTOM   2   // bottom
#define AW_LEFT     3   // left
#define AW_RIGHT    4   // right

/****************************************************************************

    FUNCTION: DrawArrow 

    PURPOSE:  Draw one arrow in a given color. 

****************************************************************************/
static
VOID DrawArrow( HDC hDC, INT type, INT xPos, INT yPos, COLORREF crPenColor )
{
    INT shaftlen=30;         // length of arrow shaft
    INT headlen=15;          // height or width of arrow head (not length)
    HPEN hPen, hPrevPen = NULL;   // pens
    INT x,y;
    INT xdir, ydir;          // directions of x and y (1,-1)

    hPen= CreatePen( PS_SOLID, 1, crPenColor );
    if( hPen )
        hPrevPen= SelectObject( hDC, hPen );

    MoveToEx( hDC, xPos, yPos, NULL );

    xdir= ydir= 1;   // defaults
    switch( type )
    {
        case AW_BOTTOM:
            ydir= -1;
        case AW_TOP:
            LineTo(hDC, xPos, yPos+ydir*shaftlen);

            for( x=0; x<3; x++ )
            {
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos-(headlen-x), yPos+ydir*headlen );
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos+(headlen-x), yPos+ydir*headlen );
            }
            break;

        case AW_RIGHT:
            xdir= -1;
        case AW_LEFT:
            LineTo( hDC, xPos + xdir*shaftlen, yPos );

            for( y=0; y<3; y++ )
            {
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos+(headlen-y));
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos-(headlen-y));
            }
            break;
    }

    if( hPrevPen )
        SelectObject( hDC, hPrevPen );

    if (hPen)
        DeleteObject(hPen);

}
/****************************************************************************

    FUNCTION: LabelRect 

    PURPOSE:  Label a rectangle with centered text given resource ID.

****************************************************************************/

static
VOID LabelRect(HDC hDC, PRECT pRect, UINT idString )
{
    UINT iStatus;
    INT xStart, yStart;
    SIZE Size;              // for size of string
    TCHAR szMsg[100];

    if( idString == 0 )     // make it easy to ignore call
        return;

    SetBkMode( hDC, OPAQUE );
    SetBkColor( hDC, RGB(0,0,0) );
    SetTextColor( hDC, RGB(255,255,255) );

    // center
    xStart= (pRect->left+pRect->right) /2;
    yStart= (pRect->top+pRect->bottom) /2;

    iStatus= LoadString( ghmod, idString, szMsg, sizeof(szMsg) );
    if( !iStatus )
    {
        return;      // can't find string - print nothing
    }

    GetTextExtentPoint32( hDC, szMsg, lstrlen(szMsg), &Size );
    TextOut( hDC, xStart-Size.cx/2, yStart-Size.cy/2, szMsg, lstrlen(szMsg) );
}

/****************************************************************************

    FUNCTION: PaintRect 

    PURPOSE:  Color in a rectangle and label it. 

****************************************************************************/
static
VOID PaintRect(
HDC hDC,         // DC to paint 
INT lowx,        // coordinates describing rectangle to fill 
INT lowy,        //  
INT highx,       // 
INT highy,       // 
COLORREF rgb,    // color to fill in rectangle with 
UINT idString )  // resource ID to use to label or 0 is none
{
    RECT rct;
    HBRUSH hBrush;

    MakeRect( &rct, lowx, lowy, highx, highy );

    hBrush= CreateSolidBrush( rgb );
    FillRect( hDC, &rct, hBrush );
    DeleteObject( hBrush );

    LabelRect( hDC, &rct, idString ); 

}

/****************************************************************************

    FUNCTION: DrawArrows 

    PURPOSE:  Draw all the arrows showing edges of resolution.

****************************************************************************/
VOID DrawArrows( HDC hDC, INT xRes, INT yRes )
{
    INT dx,dy;
    INT x,y;
    COLORREF color= RGB(0,0,0);    // color of arrow

    dx= xRes/8;
    dy= yRes/8;

    for( x=0; x<xRes; x += dx )
    {
        DrawArrow( hDC, AW_TOP,    dx/2+x,   0,      color ); 
        DrawArrow( hDC, AW_BOTTOM, dx/2+x,   yRes-1, color );
    }
    for( y=0; y<yRes; y += dy )
    {
        DrawArrow( hDC, AW_LEFT,       0, dy/2+y,   color  ); 
        DrawArrow( hDC, AW_RIGHT, xRes-1, dy/2+y,   color );
    }
}
/****************************************************************************

    FUNCTION: LabelResolution 

    PURPOSE:  Labels the resolution in a form a user may understand.
              bugbug: We could label vertically too. 

****************************************************************************/

VOID LabelResolution( HDC hDC, INT xmin, INT ymin, INT xmax, INT ymax )
{
   TCHAR szRes[120];    // text for resolution
   TCHAR szFmt[100];    // format string 
   SIZE  Size;
   INT iStatus;

   iStatus= LoadString( ghmod, IDS_RESOLUTION_FMT, szFmt, sizeof(szFmt) );
   if( !iStatus || iStatus==sizeof(szFmt) )
   { 
       lstrcpy(szFmt,TEXT("%d x %d"));   // make sure we get something
   }
   wsprintf( szRes, szFmt, xmax, ymax );

   SetBkMode( hDC, TRANSPARENT );
   SetTextColor( hDC, RGB(0,0,0) );

   GetTextExtentPoint32( hDC, szRes, lstrlen(szRes), &Size );

   // Text near bottom of screen ~10 pixels from bottom
   TextOut( hDC, xmax/2 - Size.cx/2, ymax - 10-Size.cy, szRes, lstrlen(szRes) );
} 


// table of resolutions that we show off.
// if the resolution is larger, then we show that one too.

typedef struct tagRESTAB {
    INT xRes;
    INT yRes;
    COLORREF crColor;           // color to paint this resolution
} RESTAB;

RESTAB ResTab[] ={
   { 1600, 1200, RGB(255,0,0)},
   { 1280, 1024, RGB(0,255,0)},
   { 1152,  900, RGB(0,0,255)},
   { 1024,  768, RGB(255,0,0)},
   {  800,  600, RGB(0,255,0)},
   // 640x480 or 640x400 handled specially
   { 0, 0, 0}         // end of table
   };

/****************************************************************************

    FUNCTION: Set1152Mode

    PURPOSE:  Set the height of the 1152 mode since it varies from card to
              card.

****************************************************************************/
VOID Set1152Mode(int height)
{
    ResTab[2].yRes = height;
}

/****************************************************************************

    FUNCTION: DrawBmp

    PURPOSE:  Show off a fancy screen so the user has some idea
              of what will be seen given this resolution, colour
              depth and vertical refresh rate.  Note that we do not
              try to simulate the font sizes.  

****************************************************************************/
VOID DrawBmp( HDC hDC )
{
    INT    nBpp;          // bits per pixel
    INT    nWidth;        // width of screen in pixels
    INT    nHeight;       // height of screen in pixels
    INT    xUsed,yUsed;   // amount of x and y to use for dense bitmap
    INT    dx,dy;         // delta x and y for color bars
    RECT   rct;           // rectangle for passing bounds
    HFONT  hPrevFont=0;   // previous font in DC
//    HFONT  hFont;         // stock font for logfont
    HFONT  hNewFont;      // new font if possible
//    LOGFONT lf;           // for creating new font
    HPEN   hPrevPen;      // previous pen handle
    INT    x,y,i;
    INT    off;           // offset in dx units

    // try to use bigger better looking font

    //hFont= GetStockObject( DEVICE_DEFAULT_FONT );
    //GetObject( hFont, sizeof(LOGFONT), &lf );
    //lf.lfHeight= 30;
    //hNewFont= CreateFontIndirect( &lf );
    hNewFont = (HFONT)NULL;

    if( hNewFont )                              // if no font, use old
        hPrevFont= SelectObject( hDC, hNewFont );

    // get surface information
    nBpp= GetDeviceCaps( hDC, BITSPIXEL ) * GetDeviceCaps( hDC, PLANES );
    nWidth= GetDeviceCaps( hDC, HORZRES );
    nHeight= GetDeviceCaps( hDC, VERTRES );

    // background for everything is yellow.
    PaintRect( hDC, 0,0,nWidth, nHeight, RGB(255,255,0),0 );
    LabelResolution( hDC, 0,0,nWidth, nHeight );

    // Background for various resolutions
    // biggest ones first

    for( i=0; ResTab[i].xRes !=0; i++ )
    {
        // Only draw if it will show
        if( ( nWidth>=ResTab[i].xRes ) | ( nHeight>=ResTab[i].yRes ) )
        {
           PaintRect(hDC,0,0,ResTab[i].xRes,ResTab[i].yRes,ResTab[i].crColor,0);
           LabelResolution( hDC, 0, 0, ResTab[i].xRes, ResTab[i].yRes);
        }
    }

    // color bars - only in standard vga area 

    xUsed= min( nWidth, 640 );    // only use vga width
    yUsed= min( nHeight, 480 );   // could be 400 on some boards
    dx= xUsed/2;
    dy= yUsed/6;

    PaintRect( hDC, 0,   0, dx, dy*1,  RGB(255,0,0),   IDS_COLOR_RED );
    PaintRect( hDC, 0,dy*1, dx, dy*2,  RGB(0,255,0),   IDS_COLOR_GREEN );
    PaintRect( hDC, 0,dy*2, dx, dy*3,  RGB(0,0,255),   IDS_COLOR_BLUE );
    PaintRect( hDC, 0,dy*3, dx, dy*4,  RGB(255,255,0 ),IDS_COLOR_YELLOW );
    PaintRect( hDC, 0,dy*4, dx, dy*5,  RGB(255,0,255), IDS_COLOR_MAGENTA  );
    PaintRect( hDC, 0,dy*5, dx, yUsed, RGB(0,255,255), IDS_COLOR_CYAN  );

    // gradations of colors for true color detection
    for( x=dx; x<xUsed; x++ )
    {
        INT level;
        level= 255- ( 256*(x-dx) ) / dx;
        PaintRect( hDC, x, dy*0, x+1,  dy*1, RGB( level,0,0 ),0 );
        PaintRect( hDC, x, dy*1, x+1,  dy*2, RGB( 0,level,0 ),0 );
        PaintRect( hDC, x, dy*2, x+1,  dy*3, RGB( 0,0,level ),0 );
        PaintRect( hDC, x, dy*5, x+1,  dy*6, RGB( level,level,level), 0 );
    }
    MakeRect( &rct, dx,0,dx*2,dy*1 );
    LabelRect( hDC, &rct, IDS_RED_SHADES );
    MakeRect( &rct, dx,dy,dx*2,dy*2);
    LabelRect( hDC, &rct, IDS_GREEN_SHADES );
    MakeRect( &rct, dx,2*dy,dx*2,dy*3);
    LabelRect( hDC, &rct, IDS_BLUE_SHADES );
    MakeRect( &rct, dx,5*dy,dx*2,dy*6);
    LabelRect( hDC, &rct, IDS_GRAY_SHADES );

    // horizontal lines for interlace detection
    off= 3;
    PaintRect(hDC, dx,dy*off, xUsed, dy*(off+1),RGB(255,255,255),0 );// white
    hPrevPen= SelectObject( hDC, GetStockObject(BLACK_PEN) );
    for( y=dy*off; y<dy*(off+1); y= y+2 )
    {
        MoveToEx( hDC, dx,   y, NULL );
        LineTo(   hDC, dx*2, y );
    }
    SelectObject( hDC, hPrevPen );
    MakeRect( &rct, dx, dy*off, dx*2, dy*(off+1) );
    LabelRect( hDC, &rct, IDS_PATTERN_HORZ );

    // vertical lines for bad dac detection
    off= 4;
    PaintRect(hDC, dx,dy*off, xUsed,dy*(off+1), RGB(255,255,255),0 );  // white
    hPrevPen= SelectObject( hDC, GetStockObject(BLACK_PEN) );
    for( x=dx; x<xUsed; x= x+2 )
    {
        MoveToEx( hDC, x, dy*off, NULL );
        LineTo(   hDC, x, dy*(off+1) );
    }
    SelectObject( hDC, hPrevPen );
    MakeRect( &rct, dx, dy*off, dx*2, dy*(off+1) );
    LabelRect( hDC, &rct, IDS_PATTERN_VERT );

    DrawArrows( hDC, nWidth, nHeight ); 

    LabelResolution(hDC, 0,0, xUsed, yUsed );

    // delete created font if one was created
    if( hPrevFont )
    {
        hPrevFont= SelectObject( hDC, hPrevFont );
        DeleteObject( hPrevFont );
    }

}
