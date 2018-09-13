/******************************Module*Header*******************************\
* Module Name: clear.cxx
*
* Window clearing functions
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/timeb.h>
#include <GL/gl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "ssintrnl.hxx"
#include "util.hxx"
#include "clear.hxx"

#define SS_CLEAR_BASE_DIV 32
#define SS_CLEAR_BASE_SIZE 16


/******************************Public*Routine******************************\
* ss_RectWipeClear
*
* Clears by drawing top, bottom, left, right rectangles that shrink in size
* towards the center.
*
* Calibration is used to try to maintain an ideal clear time.
*
\**************************************************************************/

int
ss_RectWipeClear( int width, int height, int repCount )
{
    int i, j, xinc, yinc, numDivs;
    int xmin, xmax, ymin, ymax;
    int w, h;
    BOOL bCalibrate = FALSE;
    double elapsed;
    static double idealTime = 0.7;
    SS_TIMER timer;

    xinc = 1;
    yinc = 1;
    numDivs = height; // assumes height <= width

    xmin = ymin = 0;
    xmax = width-1;
    ymax = height-1;

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    glEnable( GL_SCISSOR_TEST );

    if( repCount == 0 ) {
        bCalibrate = TRUE;
        repCount = 1;
        timer.Start();
    }

    for( i = 0; i < (numDivs/2); i ++ ) {
      w = xmax - xmin + 1;
      h = ymax - ymin + 1;

      for( j = 0; j < repCount; j ++ ) {
        // bottom
        glScissor( xmin, ymin, w, 1 );
        glClear( GL_COLOR_BUFFER_BIT );

        // left
        glScissor( xmin, ymin, 1, h );
        glClear( GL_COLOR_BUFFER_BIT );

        // right
        glScissor( xmax, ymin, 1, h );
        glClear( GL_COLOR_BUFFER_BIT );

        // top
        glScissor( xmin, ymax, w, 1 );
        glClear( GL_COLOR_BUFFER_BIT );

        glFlush(); // to eliminate 'bursts'
      }

      xmin += xinc;
      xmax -= xinc;
      ymin += yinc;
      ymax -= yinc;
    }

    if( bCalibrate ) {
        elapsed = timer.Stop();

        // try to maintain ideal clear time
        if( elapsed < idealTime ) {
            // increase repCount to slow down the clear
            if( elapsed == 0.0 )
                repCount = 10;  // just in case
            else
                repCount = (int) ((idealTime / elapsed) + 0.5);
        }
    }

    glDisable( GL_SCISSOR_TEST );

    return repCount;
}

#define SS_CLEAR_BASE_DIV 32

/******************************Public*Routine******************************\
*
* SS_DIGITAL_DISSOLVE_CLEAR constructor
*
\**************************************************************************/

SS_DIGITAL_DISSOLVE_CLEAR::
SS_DIGITAL_DISSOLVE_CLEAR()
{
    rectBuf = NULL;
    rectBufSize = 0;
    rectSize = SS_CLEAR_BASE_SIZE;
}

/******************************Public*Routine******************************\
*
* SS_DIGITAL_DISSOLVE_CLEAR destructor
*
\**************************************************************************/

SS_DIGITAL_DISSOLVE_CLEAR::
~SS_DIGITAL_DISSOLVE_CLEAR()
{
    if( rectBuf )
        LocalFree( rectBuf );
}

/******************************Public*Routine******************************\
* CalibrateClear
*
* Try to calibrate the clear so it takes the specified time
\**************************************************************************/

//#define AUTO_CALIBRATE 1

int SS_DIGITAL_DISSOLVE_CLEAR::
CalibrateClear( int width, int height, float fClearTime )
{
    float factor;
    int idealNRects;
    int nRects;
    int baseSize;
    SS_TIMER timer;
    float elapsed;

    baseSize = (width < height ? width : height) / SS_CLEAR_BASE_DIV;
    if( baseSize == 0 )
        baseSize = 1;

    timer.Start();

#ifdef AUTO_CALIBRATE
    // Clear a small region (quarter-screen) and extrapolate
    Clear( width >> 1, height >> 1, baseSize );
#else
    Clear( width, height, baseSize );
#endif

    elapsed = timer.Stop();
#ifdef AUTO_CALIBRATE
    // extrapolate to full screen time
    // mf: this approximation resulted in clears being somewhat less than ideal
    //  I guess this means more time than I thought was spent in scanning up
    //  or down for uncleared rects
    elapsed *= 4.0f;
#endif

    // Adjust size of rects for ideal clear time

    if( elapsed <= 0.0f ) {
        rectSize = 1;
        return rectSize;
    }

    nRects = RectangleCount( width, height, baseSize );
    factor = fClearTime / elapsed;
    idealNRects = (int) (factor * (float)nRects);
    rectSize = (int) (sqrt( (double)(width*height) / (double)idealNRects ) + 0.5);
    if( rectSize == 0 )
        rectSize = 1;

    return rectSize;
}

/******************************Public*Routine******************************\
* 
* SS_DIGITAL_DISSOLVE_CLEAR::Clear
*
* Clears by drawing random rectangles
*
\**************************************************************************/


BOOL SS_DIGITAL_DISSOLVE_CLEAR::
Clear( int width, int height )
{
    return Clear( width, height, rectSize );
}

BOOL SS_DIGITAL_DISSOLVE_CLEAR::
Clear( int width, int height, int size )
{
    BOOL *pRect;
    BOOL bCalibrate = FALSE;
    int count, nRects;
    int i, xdim, ydim;
    static float idealTime = 2.0f;

    if( (size <= 0) || !width || !height )
        return FALSE;

    // determine xdim, ydim from size
    xdim = SS_ROUND_UP( (float)width / (float)size );
    ydim = SS_ROUND_UP( (float)height / (float) size );

    // figure out how many rects needed
    count = nRects = xdim * ydim;

    // make sure enough room
    if( !ValidateBufSize( nRects ) )
        return FALSE;

    // reset the rect array to uncleared

    pRect = rectBuf;
    for( i = 0; i < count; i ++, pRect++ )
        *pRect = FALSE;

    // Clear random rectangles

    glEnable( GL_SCISSOR_TEST );

    while( count ) {
        // pick a random rect
        i = ss_iRand( nRects );

        if( rectBuf[i] ) {
            // This rect has already been cleared - find an empty one
            // Scan up and down from x,y, looking at the array linearly

            int up, down;
            BOOL searchUp = FALSE;

            up = down = i;

            pRect = rectBuf;
            while( *(pRect + i) ) {
                if( searchUp ) {
                    // search up side
                    if( up < (nRects-1) ) {
                        up++;
                    }
                    i = up;
                } else {
                    // search down side
                    if( down > 0 ) {
                        down--;
                    }
                    i = down;
                }
                searchUp = !searchUp;
            }
        }

        // clear the x,y rect
        glScissor( (i % xdim)*size, (i / xdim)*size, size, size );
        glClear( GL_COLOR_BUFFER_BIT );
        glFlush();

        rectBuf[i] = TRUE; // mark as taken
        count--;
    }

    glDisable( GL_SCISSOR_TEST );

    return TRUE;
}

/******************************Public*Routine******************************\
* RectangleCount
*
\**************************************************************************/

int SS_DIGITAL_DISSOLVE_CLEAR::
RectangleCount( int width, int height, int size )
{
    return  SS_ROUND_UP( (float)width / (float)size ) *
            SS_ROUND_UP( (float)height / (float) size );
}

/******************************Public*Routine******************************\
* ValidateBufSize
*
\**************************************************************************/

BOOL SS_DIGITAL_DISSOLVE_CLEAR::
ValidateBufSize( int nRects )
{
    if( nRects > rectBufSize ) {
        // need a bigger rect buf
        BOOL *r = (BOOL *) LocalAlloc( LMEM_FIXED, sizeof(BOOL) * nRects );
        if( !r )
            return FALSE;
        if( rectBuf )
            LocalFree( rectBuf );
        rectBuf = r;
        rectBufSize = nRects;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
* DrawGdiRect
*
* Clears the rect with the brush
\**************************************************************************/

void
DrawGdiRect( HDC hdc, HBRUSH hbr, RECT *pRect ) 
{
    if( pRect == NULL )
        return;

    FillRect( hdc, pRect, hbr );
    GdiFlush();
}

#ifdef SS_INITIAL_CLEAR
/*-----------------------------------------------------------------------
|                                                                       
|    RectWipeClear(width, height):  
|       - Does a rectangular wipe (or clear) by drawing in a sequence   
|         of rectangles using Gdi                                       
|       MOD: add calibrator capability to adjust speed for different
|            architectures
|       MOD: this can be further optimized by caching the brush
|
-----------------------------------------------------------------------*/
void 
ss_GdiRectWipeClear( HWND hwnd, int width, int height )
{
    HDC hdc;
    HBRUSH hbr;
    RECT rect;
    int i, j, xinc, yinc, numDivs = 500;
    int xmin, xmax, ymin, ymax;
    int repCount = 10;

    xinc = 1;
    yinc = 1;
    numDivs = height;
    xmin = ymin = 0;
    xmax = width;
    ymax = height;

    hdc = GetDC( hwnd );

    hbr = CreateSolidBrush( RGB( 0, 0, 0 ) );

    for( i = 0; i < (numDivs/2 - 1); i ++ ) {
      for( j = 0; j < repCount; j ++ ) {
        rect.left = xmin; rect.top = ymin;
        rect.right = xmax; rect.bottom = ymin + yinc;
        FillRect( hdc, &rect, hbr );
        rect.top = ymax - yinc;
        rect.bottom = ymax;
        FillRect( hdc, &rect, hbr );
        rect.top = ymin + yinc;
        rect.right = xmin + xinc; rect.bottom = ymax - yinc;
        FillRect( hdc, &rect, hbr );
        rect.left = xmax - xinc; rect.top = ymin + yinc;
        rect.right = xmax; rect.bottom = ymax - yinc;
        FillRect( hdc, &rect, hbr );
      }

      xmin += xinc;
      xmax -= xinc;
      ymin += yinc;
      ymax -= yinc;
    }

    // clear last square in middle

    rect.left = xmin; rect.top = ymin;
    rect.right = xmax; rect.bottom = ymax;
    FillRect( hdc, &rect, hbr );

    DeleteObject( hbr );

    ReleaseDC( hwnd, hdc );

    GdiFlush();
}
#endif // SS_INITIAL_CLEAR
