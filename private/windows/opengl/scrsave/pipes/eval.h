#ifndef __eval_h__
#define __eval_h__

#include "sscommon.h"
#include "xc.h"

#define MAX_UORDER      5 // this is per section
#define MAX_VORDER      5
#define MAX_USECTIONS   4
#define MAX_XC_PTS      (MAX_UORDER * MAX_USECTIONS)

#define TEX_ORDER       2
#define EVAL_ARC_ORDER  4
#define EVAL_CYLINDER_ORDER 2
#define EVAL_ELBOW_ORDER    4

// # of components (eg. arcs) to form a complete cross-section
#define EVAL_XC_CIRC_SECTION_COUNT 4

#define EVAL_XC_POINT_COUNT ( (EVAL_ARC_ORDER-1)*4 + 1 )

#define EVAL_CIRC_ARC_CONTROL 0.56f // for r=1

/**************************************************************************\
*
* EVAL class
*
* - Evaluator composed of one or more sections that are evaluated
*   separately with OpenGL evaluators
*
\**************************************************************************/

class EVAL {
public:
    EVAL( BOOL bTexture );
    ~EVAL();
    int         numSections;    // number of cross-sectional sections
    int         uOrder, vOrder;
        // assumed: all sections same order - uOrder is per
        // section; sections share vertex and texture control points
    int         uDiv, vDiv;    // figured out one level up ?
    POINT3D     *pts;          // vertex control points
    // - texture always order 2 for s and t (linear mapping)
    BOOL        bTexture;
    TEX_POINT2D *texPts;       // texture control points

    void        Evaluate(); // evaluate/render the object
    void        SetVertexCtrlPtsXCTranslate( POINT3D *pts, float length, 
                                             XC *xcStart, XC *xcEnd );
    void        SetTextureControlPoints( float s_start, float s_end,
                                         float t_start, float t_end );
    void        ProcessXCPrimLinear( XC *xcStart, XC *xcEnd, float length );
    void        ProcessXCPrimBendSimple( XC *xcCur, int dir, float radius );
    void        ProcessXCPrimSingularity( XC *xcCur, float length, 
                                          BOOL bOpening );
};

extern void ResetEvaluator( BOOL bTexture );

#endif // __eval_h__
