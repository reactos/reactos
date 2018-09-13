/******************************Module*Header*******************************\
* Module Name: eval.cxx
*
* Evaluator stuff
*
* Copyright (c) 1994 Microsoft Corporation
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

#include "sscommon.h"
#include "sspipes.h"
#include "eval.h"

//#define EVAL_DBG 1

typedef enum {
    X_PLANE = 0,
    Y_PLANE,
    Z_PLANE
};

#define EVAL_VSIZE 3  // vertex size in floats

#define TMAJOR_ORDER 2
#define TMINOR_ORDER 2

#define VDIM 3
#define TDIM 2

//forwards
#if EVAL_DBG
static void DrawPoints( int num, POINT3D *pts );
#endif
static void RotatePointSet( POINT3D *inPts, int numPts, float angle, int dir, 
                      float radius, POINT3D *outPts );
static void ExtrudePointSetDir( POINT3D *inPts, int numPts, float *acPts, 
                      int dir, POINT3D *outPts );


/**************************************************************************\
* EVAL
*
* Evaluator constructor
* 
\**************************************************************************/

EVAL::EVAL( BOOL bTex )
{
    bTexture = bTex; 

    // Allocate points buffer

//mf: might want to use less than max in some cases
    int size = MAX_USECTIONS * MAX_UORDER * MAX_VORDER * sizeof(POINT3D);
    pts = (POINT3D *) LocalAlloc( LMEM_FIXED, size );
    SS_ASSERT( pts != NULL, "EVAL constructor\n" );
    
    // Alloc texture points buffer

    if( bTexture ) {
        size = MAX_USECTIONS * TEX_ORDER * TEX_ORDER * sizeof(TEX_POINT2D);
        texPts = (TEX_POINT2D *) LocalAlloc( LMEM_FIXED, size );
        SS_ASSERT( texPts != NULL, "EVAL constructor\n" );
    }
    
    ResetEvaluator( bTexture );
}

/**************************************************************************\
* ~EVAL
*
* Evaluator destructor
*
* Frees up memory
*
\**************************************************************************/

EVAL::~EVAL( )
{
    LocalFree( pts );
    if( bTexture )
        LocalFree( texPts );
}

/**************************************************************************\
* Reset
*
* Reset evaluator to generate 3d vertices and vertex normals
*
\**************************************************************************/

void
ResetEvaluator( BOOL bTexture )
{
    if( bTexture ) {
        glEnable( GL_MAP2_TEXTURE_COORD_2 );
    }
    glEnable( GL_MAP2_VERTEX_3 );
    glEnable( GL_AUTO_NORMAL );
    glFrontFace( GL_CW ); // cuz
//mf: !!! if mixing Normal and Flex, have to watch out for this, cuz normal
// needs CCW
}

/**************************************************************************\
* SetTextureControlPoints
*
* Set texture control point net
*
* This sets up 'numSections' sets of texture coordinate control points, based
* on starting and ending s and t values.
*
* s coords run along pipe direction, t coords run around circumference
*
* History
*  July 17, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
EVAL::SetTextureControlPoints( float s_start, float s_end, 
                              float t_start, float t_end )
{
    int i;
    TEX_POINT2D *ptexPts = texPts;
    GLfloat t_delta = (t_end - t_start) / numSections;
    GLfloat t = t_start;

    // calc ctrl pts for each quadrant
    for( i = 0; i < numSections; i++, ptexPts += (TDIM*TDIM) ) {
        // s, t coords
        ptexPts[0].t = ptexPts[2].t = t;
        t += t_delta;
        ptexPts[1].t = ptexPts[3].t = t;
        ptexPts[0].s = ptexPts[1].s = s_start;
        ptexPts[2].s = ptexPts[3].s = s_end;
    } 
}

/**************************************************************************\
* SetVertexCtrlPtsXCTranslate
*
* Builds 3D control eval control net from 2 xcObjs displaced along the
* z-axis by 'length'.
* 
* First xc used to generate points in z=0 plane.
* Second xc generates points in z=length plane.
* ! Replicates the last point around each u.
*
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
EVAL::SetVertexCtrlPtsXCTranslate( POINT3D *pts, float length, 
                             XC *xcStart, XC *xcEnd )
{
    int i;
    POINT2D *ptsStart, *ptsEnd;
    POINT3D *pts1, *pts2;
    int     numPts = xcStart->numPts;

    numPts++;  // due to last point replication

    ptsStart = xcStart->pts;
    ptsEnd   = xcEnd->pts;
    pts1     = pts;
    pts2     = pts + numPts;

    for( i = 0; i < (numPts-1); i++, pts1++, pts2++ ) {
        // copy over x,y from each xc
        *( (POINT2D *) pts1) = *ptsStart++;
        *( (POINT2D *) pts2) = *ptsEnd++;
        // set z for each
        pts1->z = 0.0f;
        pts2->z = length;
    }

    // Replicate last point in each u-band
    *pts1 = *pts;
    *pts2 = *(pts + numPts);
}

/**************************************************************************\
* ProcessXCPrimLinear
*
* Processes a prim according to evaluator data
*
* - Only valid for colinear xc's (along z)
* - XC's may be identical (extrusion).  If not identical, may have
*   discontinuities at each end.
* - Converts 2D XC pts to 3D pts
*
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
EVAL::ProcessXCPrimLinear( XC *xcStart, XC *xcEnd, float length )
{
    if( length <= 0.0f )
        // nuttin' to do
        return;

    // Build a vertex control net from 2 xcObj's a distance 'length' apart
    // this will displace the end xcObj a distance 'length' down the z-axis
    SetVertexCtrlPtsXCTranslate( pts, length, xcStart, xcEnd );

    Evaluate( );
}

/**************************************************************************\
* ProcessXCPrimBendSimple
*
* Processes a prim by bending along dir from xcCur
*
* - dir is relative from xc in x-y plane
* - adds C2 continuity at ends
*
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
EVAL::ProcessXCPrimBendSimple( XC *xcCur, int dir, float radius )
{
    POINT3D *ptsSrc, *ptsDst;
    static float acPts[MAX_XC_PTS+1];
    int ptSetStride = xcCur->numPts + 1; // pt stride for output pts buffer

    // We will be creating 4 cross-sectional control point sets here.

    // Convert 2D pts in xcCur to 3D pts at z=0 for 1st point set
    xcCur->ConvertPtsZ( pts, 0.0f );

    // Calc 4th point set by rotating 1st set as per dir
    ptsDst = pts + 3*ptSetStride;
    RotatePointSet( pts, ptSetStride, 90.0f, dir, radius, ptsDst );

    // angles != 90, hard, cuz not easy to extrude 3rd set from 4th

    // Next, have to figure out ac values.  Need to extend each xc's points
    // into bend to generate ac net.  For circular bend (and later for general
    // case elliptical bend), need to know ac distance from xc for each point.
    // This is based on the point's turn radius - a function of its distance
    // from the 'hinge' of the turn.

    // Can take advantage of symmetry here.  Figure for one xc, good for 2nd.
    // This assumes 90 deg turn.  (also,last point replicated)
    xcCur->CalcArcACValues90( dir, radius, acPts );
    
    // 2) extrude each point's ac from xcCur (extrusion in +z)
    // apply values to 1st to get 2nd
    // MINUS_Z, cuz subtracts *back* from dir
    ExtrudePointSetDir( pts, ptSetStride, acPts, MINUS_Z, 
                                                    pts + ptSetStride );

    // 3) extrude each point's ac from xcEnd (extrusion in -dir)
    ptsSrc = pts + 3*ptSetStride;
    ptsDst = pts + 2*ptSetStride;
    ExtrudePointSetDir( ptsSrc, ptSetStride, acPts, dir, ptsDst );

    Evaluate();
}

/**************************************************************************\
* eval_ProcessXCPrimSingularity
*
* Processes a prim by joining singularity to an xc
*
* - Used for closing or opening the pipe
* - If bOpening is true, starts with singularity, otherwise ends with one
* - the xc side is always in z=0 plane
* - singularity side is radius on either side of xc
* - adds C2 continuity at ends (perpendicular to +z at singularity end)
*
*  July 29, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
EVAL::ProcessXCPrimSingularity( XC *xcCur, float length, BOOL bOpening )
{
    POINT3D *ptsSing, *ptsXC;
    static float acPts[MAX_XC_PTS+1];
    float zSing; // z-value at singularity
    int ptSetStride = xcCur->numPts + 1; // pt stride for output pts buffer
    int i;
    XC xcSing(xcCur);

    // create singularity xc - which is an extremely scaled-down version
    //  of xcCur (this prevents any end-artifacts, unless of course we were
    //  to zoom it ultra-large).

    xcSing.Scale( .0005f );

    // We will be creating 4 cross-sectional control point sets here.
    // mf: 4 is like hard coded; what about for different xc component levels ?

    if( bOpening ) {
        ptsSing = pts;
        ptsXC = pts + 3*ptSetStride;
    } else {
        ptsSing = pts + 3*ptSetStride;
        ptsXC = pts;
    }

    // Convert 2D pts in xcCur to 3D pts at 'xc' point set
    xcCur->ConvertPtsZ( ptsXC, 0.0f );

    // Set z-value for singularity point set
    zSing = bOpening ? -length : length;
    xcSing.ConvertPtsZ( ptsSing, zSing );

    // The arc control for each point is based on a radius value that is
    //  each xc point's distance from the xc center
    xcCur->CalcArcACValuesByDistance( acPts );

    // Calculate point set near xc
    if( bOpening )
        ExtrudePointSetDir( ptsXC, ptSetStride, acPts, PLUS_Z, 
                                                    ptsXC - ptSetStride );
    else
        ExtrudePointSetDir( ptsXC, ptSetStride, acPts, MINUS_Z, 
                                                    ptsXC + ptSetStride );

    // Point set near singularity is harder, as the points must generate
    // a curve between the singularity and each xc point
    // No, easier, just scale each point by universal arc controller !
    POINT3D *ptsDst = pts;
    ptsDst = bOpening ? ptsSing + ptSetStride : ptsSing - ptSetStride;
    for( i = 0; i < ptSetStride; i ++, ptsDst++ ) {
        ptsDst->x = EVAL_CIRC_ARC_CONTROL * ptsXC[i].x;
        ptsDst->y = EVAL_CIRC_ARC_CONTROL * ptsXC[i].y;
        ptsDst->z = zSing;
    }

    Evaluate();
}

/**************************************************************************\
* Evaluate
*
* Evaluates the EVAL object
*
* - There may be 1 or more lengthwise sections around an xc
* - u is minor, v major
* - u,t run around circumference, v,s lengthwise
* - Texture maps are 2x2 for each section
* - ! uDiv is per section !
*
* History
*  July 21, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
EVAL::Evaluate( )
{
    int i;
    POINT3D *ppts = pts; 
    TEX_POINT2D *ptexPts = texPts;
    // total # pts in cross-section:
    int xcPointCount = (uOrder-1)*numSections + 1;

    for( i = 0; i < numSections; i ++, 
                                       ppts += (uOrder-1),
                                       ptexPts += (TEX_ORDER*TEX_ORDER) ) {

        // map texture coords

        if( bTexture ) {
            glMap2f(GL_MAP2_TEXTURE_COORD_2, 
                    0.0f, 1.0f, TDIM, TEX_ORDER, 
                    0.0f, 1.0f, TEX_ORDER*TDIM, TEX_ORDER, 
                    (GLfloat *) ptexPts );
        }

        // map vertices

        glMap2f(GL_MAP2_VERTEX_3, 
               0.0f, 1.0f, VDIM, uOrder, 
               0.0f, 1.0f, xcPointCount*VDIM, vOrder,
               (GLfloat *) ppts );

        // evaluate

        glMapGrid2f(uDiv, 0.0f, 1.0f, vDiv, 0.0f, 1.0f);
        glEvalMesh2( GL_FILL, 0, uDiv, 0, vDiv);
    }
}

#if EVAL_DBG
/**************************************************************************\
* DrawPoints
*
* draw control points
*
\**************************************************************************/
static 
void DrawPoints( int num, POINT3D *pts )
{
    GLint i;

    // draw green pts for now
    glColor3f(0.0f, 1.0f, 0.0f);
    glPointSize(2);

    glBegin(GL_POINTS);
        for (i = 0; i < num; i++, pts++) {
            glVertex3fv( (GLfloat *) pts );
        }
    glEnd();
}
#endif

/**************************************************************************\
* ExtrudePointSetDir
*
* Extrude a point set back from the current direction
*
* Generates C2 continuity at the supplied point set xc, by generating another
* point set back of the first, using supplied subtraction values.
*
*  July 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static void
ExtrudePointSetDir( POINT3D *inPts, int numPts, float *acPts, int dir, 
                    POINT3D *outPts )
{
    int i;
    float sign;
    int offset;

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
        case PLUS_Z:
            offset = 2;
            sign = -1.0f;
            break;
        case MINUS_Z:
            offset = 2;
            sign =  1.0f;
            break;
    }

    for( i = 0; i < numPts; i++, inPts++, outPts++, acPts++ ) {
        *outPts = *inPts;
        ((float *)outPts)[offset] = ((float *)inPts)[offset] + (sign * (*acPts));
    }
}

/**************************************************************************\
* RotatePointSet
*
* Rotate point set by angle, according to dir and radius
*
* - Put points in supplied outPts buffer
*
*  July 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static void 
RotatePointSet( POINT3D *inPts, int numPts, float angle, int dir, 
                      float radius, POINT3D *outPts )
{
    MATRIX matrix1, matrix2, matrix3;
    int i;
    POINT3D rot = {0.0f};
    POINT3D anchor = {0.0f};

    /* dir      rot
       +x       90 y
       -x       -90 y
       +y       -90 x
       -y       90 x
    */

    // convert angle to radians
    //mf: as noted in objects.c, we have to take negative angle to make
    // it work in familiar 'CCW rotation is positive' mode.  The ss_* rotate
    // routines must work in the 'CW is +'ve' mode, as axis pointing at you.
    angle = SS_DEG_TO_RAD(-angle);

    // set axis rotation and anchor point

    switch( dir ) {
        case PLUS_X:
            rot.y = angle;
            anchor.x = radius;
            break;
        case MINUS_X:
            rot.y = -angle;
            anchor.x = -radius;
            break;
        case PLUS_Y:
            rot.x = -angle;
            anchor.y = radius;
            break;
        case MINUS_Y:
            rot.x = angle;
            anchor.y = -radius;
            break;
    }

    // translate anchor point to origin
    ss_matrixIdent( &matrix1 );
    ss_matrixTranslate( &matrix1, -anchor.x, -anchor.y, -anchor.z );

    // rotate 
    ss_matrixIdent( &matrix2 );
    ss_matrixRotate( &matrix2, (double) rot.x, rot.y, rot.z );

    // concat these 2
    ss_matrixMult( &matrix3, &matrix2, &matrix1 );

    // translate back
    ss_matrixIdent( &matrix2 );
    ss_matrixTranslate( &matrix2,  anchor.x,  anchor.y,  anchor.z );

    // concat these 2
    ss_matrixMult( &matrix1, &matrix2, &matrix3 );

    for( i = 0; i < numPts; i ++, outPts++, inPts++ ) {
        ss_xformPoint( outPts, inPts, &matrix1 );
    }
}
