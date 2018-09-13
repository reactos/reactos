/******************************Module*Header*******************************\
* Module Name: xc.cxx
*
* Cross-section (xc) object stuff
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glaux.h>
#include <float.h>

#include "sscommon.h"
#include "sspipes.h"
#include "eval.h"
#include "xc.h"

/**************************************************************************\
* XC::CalcArcACValues90
*
* Calculate arc control points for a 90 degree rotation of an xc
*
* Arc is a quarter-circle
* - 90 degree is much easier, so we special case it
* - radius is distance from xc-origin to hinge of turn
*
* History
*  July 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
XC::CalcArcACValues90( int dir, float radius, float *acPts )
{
    int i;
    float sign;
    int offset;
    float *ppts = (float *) pts;

    // 1) calc 'r' values for each point (4 turn possibilities/point).  From
    //  this can determine ac, which is extrusion of point from xc face

    switch( dir ) {
        case PLUS_X:
            offset = 0;
            sign = -1.0f;
            break;
        case MINUS_X:
            offset = 0;
            sign =  1.0f;
            break;
        case PLUS_Y:
            offset = 1;
            sign = -1.0f;
            break;
        case MINUS_Y:
            offset = 1;
            sign =  1.0f;
            break;
    }

    for( i = 0; i < numPts; i++, ppts+=2, acPts++ ) {
        *acPts = EVAL_CIRC_ARC_CONTROL * (radius + (sign * ppts[offset]));
    }
    // replicate !
    *acPts = *(acPts - numPts);
}

/**************************************************************************\
* XC::CalcArcACValuesByDistance
*
* Use the distance of each xc point from the xc origin, as the radius for
* an arc control value.
*
* History
*  July 29, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
XC::CalcArcACValuesByDistance( float *acPts )
{
    int i;
    float r;
    POINT2D *ppts =  pts;

    for( i = 0; i < numPts; i++, ppts++ ) {
        r = (float) sqrt( ppts->x*ppts->x + ppts->y*ppts->y );
        *acPts++ = EVAL_CIRC_ARC_CONTROL * r;
    }
    // replicate !
    *acPts = *(acPts - numPts);
}

/**************************************************************************\
* ELLIPTICAL_XC::SetControlPoints
*
* Set the 12 control points for a circle at origin in z=0 plane
*
* Points go CCW from +x
*
* History
*  July 24, 95 : [marcfo]
*    - Wrote it
*  - 10/18/95 [marcfo] : Convert to C++
*
\**************************************************************************/

void 
ELLIPTICAL_XC::SetControlPoints( GLfloat r1, GLfloat r2 )
{
    GLfloat ac1, ac2; 

    ac1 = EVAL_CIRC_ARC_CONTROL * r2;
    ac2 = EVAL_CIRC_ARC_CONTROL * r1;

    // create 12-pt. set CCW from +x

    // last 2 points of right triplet
    pts[0].x = r1;
    pts[0].y = 0.0f;
    pts[1].x = r1;
    pts[1].y = ac1;

    // top triplet
    pts[2].x = ac2;
    pts[2].y = r2;
    pts[3].x = 0.0f;
    pts[3].y = r2;
    pts[4].x = -ac2;
    pts[4].y = r2;

    // left triplet
    pts[5].x = -r1;
    pts[5].y = ac1;
    pts[6].x = -r1;
    pts[6].y = 0.0f;
    pts[7].x = -r1;
    pts[7].y = -ac1;

    // bottom triplet
    pts[8].x = -ac2;
    pts[8].y = -r2;
    pts[9].x = 0.0f;
    pts[9].y = -r2;
    pts[10].x = ac2;
    pts[10].y = -r2;

    // first point of first triplet
    pts[11].x = r1;
    pts[11].y = -ac1;
}

/**************************************************************************\
* RANDOM4ARC_XC::SetControlPoints
*
* Set random control points for xc
* Points go CCW from +x
*
* History
*  July 30, 95 : [marcfo]
*    - Wrote it
*  - 10/18/95 [marcfo] : Convert to C++
*
\**************************************************************************/

void 
RANDOM4ARC_XC::SetControlPoints( float radius )
{
    int i;
    GLfloat r[4];
    float rMin = 0.5f * radius;
    float distx, disty;

    // figure the radius of each side first

    for( i = 0; i < 4; i ++ )
        r[i] = ss_fRand( rMin, radius );

    // The 4 r's now describe a box around the origin - this restricts stuff

    // Now need to select a point along each edge of the box as the joining
    // points for each arc (join points are at indices 0,3,6,9)

    pts[0].x = r[RIGHT];
    pts[3].y = r[TOP];
    pts[6].x = -r[LEFT];
    pts[9].y = -r[BOTTOM];

    // quarter of distance between edges
    disty = (r[TOP] - -r[BOTTOM]) / 4.0f;
    distx = (r[RIGHT] - -r[LEFT]) / 4.0f;
    
    // uh, put'em somwhere in the middle half of each side
    pts[0].y = ss_fRand( -r[BOTTOM] + disty, r[TOP] - disty );
    pts[6].y = ss_fRand( -r[BOTTOM] + disty, r[TOP] - disty );
    pts[3].x = ss_fRand( -r[LEFT] + distx, r[RIGHT] - distx );
    pts[9].x = ss_fRand( -r[LEFT] + distx, r[RIGHT] - distx );

    // now can calc ac's
    // easy part first:
    pts[1].x = pts[11].x = pts[0].x;
    pts[2].y = pts[4].y = pts[3].y;
    pts[5].x = pts[7].x = pts[6].x;
    pts[8].y = pts[10].y = pts[9].y;

    // right side ac's
    disty = (r[TOP] - pts[0].y) / 4.0f;
    pts[1].y = ss_fRand( pts[0].y + disty, r[TOP] );
    disty = (pts[0].y - -r[BOTTOM]) / 4.0f;
    pts[11].y = ss_fRand( -r[BOTTOM], pts[0].y - disty );

    // left side ac's
    disty = (r[TOP] - pts[6].y) / 4.0f;
    pts[5].y = ss_fRand( pts[6].y + disty, r[TOP]);
    disty = (pts[6].y - -r[BOTTOM]) / 4.0f;
    pts[7].y = ss_fRand( -r[BOTTOM], pts[6].y - disty );

    // top ac's
    distx = (r[RIGHT] - pts[3].x) / 4.0f;
    pts[2].x = ss_fRand( pts[3].x + distx, r[RIGHT] );
    distx = (pts[3].x - -r[LEFT]) / 4.0f;
    pts[4].x = ss_fRand( -r[LEFT],  pts[3].x - distx );

    // bottom ac's
    distx = (r[RIGHT] - pts[9].x) / 4.0f;
    pts[10].x = ss_fRand( pts[9].x + distx, r[RIGHT] );
    distx = (pts[9].x - -r[LEFT]) / 4.0f;
    pts[8].x = ss_fRand( -r[LEFT], pts[9].x - distx );
}


/**************************************************************************\
* ConvertPtsZ
*
* Convert the 2D pts in an xc, to 3D pts in point buffer, with z.
*
* Also replicate the last point.
*
*  July 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
XC::ConvertPtsZ( POINT3D *newpts, float z )
{
    int i;
    POINT2D *xcPts = pts;

    for( i = 0; i < numPts; i++, newpts++ ) {
        *( (POINT2D *) newpts ) = *xcPts++;
        newpts->z = z;
    }
    *newpts = *(newpts - numPts);
}

/**************************************************************************\
* XC::CalcBoundingBox
* 
* Calculate bounding box in x/y plane for xc
*
*  July 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/


void 
XC::CalcBoundingBox( )
{
    POINT2D *ppts = pts;
    int i;
    float xMin, xMax, yMax, yMin;

    // initialize to really insane numbers
    xMax = yMax = -FLT_MAX;
    xMin = yMin = FLT_MAX;

    // compare with rest of points
    for( i = 0; i < numPts; i ++, ppts++ ) {
        if( ppts->x < xMin )
            xMin = ppts->x;
        else if( ppts->x > xMax )
            xMax = ppts->x;
        if( ppts->y < yMin )
            yMin = ppts->y;
        else if( ppts->y > yMax )
            yMax = ppts->y;
    }
    xLeft = xMin;
    xRight = xMax;
    yBottom = yMin;
    yTop = yMax;
}

/**************************************************************************\
*
* MinTurnRadius
*
* Get minimum radius for the xc to turn in given direction. 
*
* If the turn radius is less than this minimum, then primitive will 'fold'
* over itself at the inside of the turn, creating ugliness.
*
* History
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

float
XC::MinTurnRadius( int relDir )
{
//mf: for now, assume xRight, yTop positive, xLeft, yBottom negative
// otherwise, might want to consider 'negative'radius
    switch( relDir ) {
        case PLUS_X:
            return( xRight );
        case MINUS_X:
            return( - xLeft );
        case PLUS_Y:
            return( yTop );
        case MINUS_Y:
            return( - yBottom );
        default:
            return(0.0f);
    }
}
/**************************************************************************\
*
* XC::MaxExtent
*
* Get maximum extent of the xc in x and y
*
* History
*  Aug. 1, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

float
XC::MaxExtent( )
{
    float max;

    max = xRight;

    if( yTop > max )
        max = yTop;
    if( -xLeft > max )
        max = -xLeft;
    if( -yBottom > max )
        max = -yBottom;

    return max;
}

/**************************************************************************\
* 
* XC::Scale
* 
* Scale an XC's points and extents by supplied scale value
*
\**************************************************************************/

void
XC::Scale( float scale )
{
    int i;
    POINT2D *ppts = pts;

    for( i = 0; i < numPts; i ++, ppts++ ) {
        ppts->x *= scale;
        ppts->y *= scale;
    }

    xLeft *= scale;
    xRight *= scale;
    yBottom *= scale;
    yTop *= scale;
}

/**************************************************************************\
* ~XC::XC
*
* Destructor
*
\**************************************************************************/

XC::~XC()
{
    if( pts )
        LocalFree( pts );
}

/**************************************************************************\
* XC::XC
*
* Constructor
*
* - Allocates point buffer for the xc
*
\**************************************************************************/

XC::XC( int nPts )
{
    numPts = nPts;
    pts = (POINT2D *)  LocalAlloc( LMEM_FIXED, numPts * sizeof(POINT2D) );
    SS_ASSERT( pts != 0, "XC constructor\n" );
}

#if 0
/**************************************************************************\
* XC::XC
*
* Constructor
*
* - Copies data from another XC
*
\**************************************************************************/

//mf: couldn't get calling this to work (compile time)
XC::XC( const XC& xc )
{
    numPts = xc.numPts;
    pts = (POINT2D *)  LocalAlloc( LMEM_FIXED, numPts * sizeof(POINT2D) );
    SS_ASSERT( pts != 0, "XC constructor\n" );
    RtlCopyMemory( pts, xc.pts, numPts * sizeof(POINT2D) );

    xLeft = xc.xLeft;
    xRight = xc.xRight;
    yBottom = xc.yBottom;
    yTop = xc.yTop;
}
#endif

XC::XC( XC *xc )
{
    numPts = xc->numPts;
    pts = (POINT2D *)  LocalAlloc( LMEM_FIXED, numPts * sizeof(POINT2D) );
    SS_ASSERT( pts != 0, "XC constructor\n" );
    RtlCopyMemory( pts, xc->pts, numPts * sizeof(POINT2D) );

    xLeft = xc->xLeft;
    xRight = xc->xRight;
    yBottom = xc->yBottom;
    yTop = xc->yTop;
}

/**************************************************************************\
*
* ELLIPTICAL_XC::ELLIPTICALXC
*
* Elliptical XC constructor

* These have 4 sections of 4 pts each, with pts shared between sections.
*
\**************************************************************************/

ELLIPTICAL_XC::ELLIPTICAL_XC( float r1, float r2 )
    // initialize base XC with numPts
    : XC( (int) EVAL_XC_CIRC_SECTION_COUNT * (EVAL_ARC_ORDER - 1))
{
    SetControlPoints( r1, r2 );
    CalcBoundingBox( );
}

/**************************************************************************\
*
* RANDOM4ARC_XC::RANDOM4ARC_XC
*
* Random 4-arc XC constructor

* The bounding box is 2*r each side
* These have 4 sections of 4 pts each, with pts shared between sections.
*
\**************************************************************************/
RANDOM4ARC_XC::RANDOM4ARC_XC( float r )
    // initialize base XC with numPts
    : XC( (int) EVAL_XC_CIRC_SECTION_COUNT * (EVAL_ARC_ORDER - 1))
{
    SetControlPoints( r );
    CalcBoundingBox( );
}

