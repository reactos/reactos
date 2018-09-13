/******************************Module*Header*******************************\
* Module Name: fpipe.cxx
*
* Flex pipes
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

/* Notes:
    
    - All Draw routines start with current xc at the beginning, and create
      a new one at the end.  Since it is common to just have 2 xc's for
      each prim, xcCur holds the current xc, and xcEnd is available
      for the draw routine to use as the end xc.
        They also reset xcCur when done
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>

#include "sspipes.h"
#include "fpipe.h"
#include "eval.h"

// defCylNotch shows the absolute notch for the default cylinder,
// given a direction (notch is always along +x axis)

static GLint defCylNotch[NUM_DIRS] = 
        { MINUS_Z, PLUS_Z, PLUS_X, PLUS_X, PLUS_X, MINUS_X };

static int GetRelativeDir( int lastDir, int notchVec, int newDir );

/**************************************************************************\
* FLEX_PIPE constructor
*
*
\**************************************************************************/

FLEX_PIPE::FLEX_PIPE( STATE *pState )
: PIPE( pState )
{
    float circ;

    // Create an EVAL object

    nSlices = pState->nSlices;

    // No XC's yet, they will be allocated at pipe Start()
    xcCur = xcEnd = NULL;

    // The EVAL will be used for all pEvals in the pipe, so should be
    // set to hold max. possible # of pts for the pipe.
    pEval = new EVAL( bTexture );

    // Determine pipe tesselation
    // For now, this is based on global tesselation factor

    //mf: maybe clean up this scheme a bit
    // Calculate evalDivSize, a reference value for the size of a UxV division.
    // This is used later for calculating texture coords.
    circ = CIRCUMFERENCE( pState->radius );
    evalDivSize = circ / (float) nSlices;
}

/**************************************************************************\
* ~FLEX_PIPE
*
\**************************************************************************/

FLEX_PIPE::~FLEX_PIPE( )
{
    delete pEval;

    // delete any XC's
    if( xcCur != NULL ) {
        if( xcEnd == xcCur )
            //mf: so far this can't happen...
            xcEnd = NULL; // xcCur and xcEnd can point to same xc !
        delete xcCur;
        xcCur = NULL;
    }

    if( xcEnd != NULL ) {
        delete xcEnd;
        xcEnd = NULL;
    }
}

/**************************************************************************\
* REGULAR_FLEX_PIPE constructor
*
\**************************************************************************/

REGULAR_FLEX_PIPE::REGULAR_FLEX_PIPE( STATE *state )
: FLEX_PIPE( state )
{
    static float turnFactorRange = 0.1f;
    type = TYPE_FLEX_REGULAR;

    // figure out turning factor range (0 for min bends, 1 for max bends)
#if 1
    float avgTurn = ss_fRand( 0.11f, 0.81f );
    // set min and max turn factors, and clamp to 0..1
    turnFactorMin = 
                SS_CLAMP_TO_RANGE( avgTurn - turnFactorRange, 0.0f, 1.0f );
    turnFactorMax = 
                SS_CLAMP_TO_RANGE( avgTurn + turnFactorRange, 0.0f, 1.0f );
#else
// debug: test max bend
    turnFactorMin = turnFactorMax = 1.0f;
#endif
    // choose straight weighting
    // mf:for now, same as npipe - if stays same, put in pipe
    if( ! ss_iRand( 20 ) )
        weightStraight = ss_iRand2( MAX_WEIGHT_STRAIGHT/4, MAX_WEIGHT_STRAIGHT );
    else
        weightStraight = ss_iRand( 4 );
}

/**************************************************************************\
* TURNING_FLEX_PIPE constructor
*
\**************************************************************************/

TURNING_FLEX_PIPE::TURNING_FLEX_PIPE( STATE *state )
: FLEX_PIPE( state )
{
    type = TYPE_FLEX_TURNING;
}

/******************************Public*Routine******************************\
* SetTexIndex
*
* Set the texture index for this pipe, and calculate texture state dependent
* on texRep values
*
* Dec. 95 [marcfo]
*
\**************************************************************************/

void
FLEX_PIPE::SetTexParams( TEXTURE *pTex, IPOINT2D *pTexRep )
{
    if( bTexture ) {
        GLfloat t_size;
        float circ;

        t_start = (GLfloat) pTexRep->y * 1.0f;
        t_end = 0.0f;

        // calc height (t_size) of one rep of texture around circumference
        circ = CIRCUMFERENCE( radius );
        t_size = circ / pTexRep->y;

        // now calc corresponding width of the texture using its x/y ratio
        s_length = t_size / pTex->origAspectRatio;
        s_start = s_end = 0.0f;
//mf: this means we are 'standardizing' the texture size and proportions
// on pipe of radius 1.0 for entire program.  Might want to recalc this on
// a per-pipe basis ?
    }
}

/**************************************************************************\
* ChooseXCProfile
*
* Initialize extruded pipe scheme.  This uses a randomly constructed XC, but it
* remains constant throughout the pipe
*
* History
*  July 30, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
FLEX_PIPE::ChooseXCProfile()
{
    static float turnFactorRange = 0.1f;
    float avgTurn;
    float baseRadius = pState->radius;

    // initialize evaluator elements:

    pEval->numSections = EVAL_XC_CIRC_SECTION_COUNT;
    pEval->uOrder = EVAL_ARC_ORDER;
//mf: watch this - maybe should ROUND_UP uDiv
    // set uDiv per section (assumed uDiv multiple of numSections)
    pEval->uDiv = nSlices / pEval->numSections;

    // Setup XC's

    // The xc profile remains constant throughout in this case,
    // so we only need one xc.

    // Choose between elliptical or random cross-sections.  Since elliptical
    //  looks a little better, make it more likely
    if( ss_iRand(4) )  // 3/4 of the time
        xcCur = new ELLIPTICAL_XC( ss_fRand(1.2f, 2.0f) * baseRadius, 
                                           baseRadius );
    else
        xcCur = new RANDOM4ARC_XC( ss_fRand(1.5f, 2.0f) * baseRadius );
}


/**************************************************************************\
* REGULAR_FLEX_PIPE::Start
*
* Does startup of extruded-XC pipe drawing scheme 
*
\**************************************************************************/

void
REGULAR_FLEX_PIPE::Start()
{
    NODE_ARRAY *nodes = pState->nodes;
    int newDir;

    // Set start position

    if( !SetStartPos() ) {
        status = PIPE_OUT_OF_NODES;
        return;
    }

    // set material

    ChooseMaterial();

    // set XC profile

    ChooseXCProfile();

    // push matrix with zTrans and scene rotation

    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // set random lastDir
    lastDir = ss_iRand( NUM_DIRS );

    // get a new node to draw to
    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) {
        // draw like one of those tea-pouring thingies...
        status = PIPE_STUCK;
        DrawTeapot();
        glPopMatrix();
        return;
    } else
        status = PIPE_ACTIVE;

    align_plusz( newDir ); // get us pointed in right direction

    // draw start cap, which will end right at current node
    DrawCap( START_CAP );

    // set initial notch vector, which is just the default notch, since
    // we didn't have to spin the start cap around z
    notchVec = defCylNotch[newDir];

    zTrans = - pState->view.divSize;  // distance back from new node

    UpdateCurrentPosition( newDir );

    lastDir = newDir;
}

/**************************************************************************\
* TURNING_FLEX_PIPE::Start
*
* Does startup of turning extruded-XC pipe drawing scheme 
*
\**************************************************************************/

void
TURNING_FLEX_PIPE::Start( )
{
    NODE_ARRAY *nodes = pState->nodes;

    // Set start position

    if( !SetStartPos() ) {
        status = PIPE_OUT_OF_NODES;
        return;
    }

    // Set material

    ChooseMaterial();

    // Set XC profile

    ChooseXCProfile();

    // Push matrix with zTrans and scene rotation

    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // lastDir has to be set to something valid, in case we get stuck right
    // away, cuz Draw() will be called anyways on next iteration, whereupon
    // it finds out it really is stuck, AFTER calling ChooseNewTurnDirection,
    // which requires valid lastDir. (mf: fix this)
    lastDir = ss_iRand( NUM_DIRS );

    // Pick a starting direction by finding a neihgbouring empty node
    int newDir = nodes->FindClearestDirection( &curPos );
    // We don't 'choose' it, or mark it as taken, because ChooseNewDirection
    // will always check it anyways


    if( newDir == DIR_NONE ) {
        // we can't go anywhere
        // draw like one of those tea-pouring thingies...
        status = PIPE_STUCK;
        DrawTeapot();
        glPopMatrix();
        return;
    } else
        status = PIPE_ACTIVE;

    align_plusz( newDir ); // get us pointed in right direction

    // Draw start cap, which will end right at current node
    DrawCap( START_CAP );

    // Set initial notch vector, which is just the default notch, since
    // we didn't have to spin the start cap around z
    notchVec = defCylNotch[newDir];

    zTrans = 0.0f;  // right at current node

    lastDir = newDir;
}

/**************************************************************************\
* REGULAR_FLEX_PIPE::Draw
*
* Draws the pipe using a constant random xc that is extruded
*
* Minimum turn radius can vary, since xc is not symmetrical across any
* of its axes.  Therefore here we draw using a pipe/elbow sequence, so we
* know what direction we're going in before drawing the elbow.  The current
* node is the one we will draw thru next time.  Typically, the actual end
* of the pipe is way back of this node, almost at the previous node, due
* to the variable turn radius
*
* History
*  July 30, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
REGULAR_FLEX_PIPE::Draw( )
{
    float turnRadius, minTurnRadius;
    float pipeLen, maxPipeLen, minPipeLen;
    int newDir, relDir;
    float maxXCExtent;
    NODE_ARRAY *nodes = pState->nodes;
    float divSize = pState->view.divSize;

    // get new direction

    newDir = ChooseNewDirection();
    if( newDir == DIR_NONE ) {
        status = PIPE_STUCK;
        DrawCap( END_CAP );
        glPopMatrix();
        return;
    }

    // draw pipe, and if turning, joint

    if( newDir != lastDir ) { // turning! - we have to draw joint

        // get relative turn, to figure turn radius

        relDir = GetRelativeDir( lastDir, notchVec, newDir );
        minTurnRadius = xcCur->MinTurnRadius( relDir );

        // now calc maximum straight section we can draw before turning
        // zTrans is current pos'n of end of pipe, from current node ??
        // zTrans is current pos'n of end of pipe, from last node

        maxPipeLen = (-zTrans) - minTurnRadius;

        // there is also a minimum requirement for the length of the straight
        // section, cuz if we turn too soon with a large turn radius, we
        // will swing up too close to the next node, and won't be able to
        // make one or more of the 4 possible turns from that point

        maxXCExtent = xcCur->MaxExtent(); // in case need it again
        minPipeLen = maxXCExtent - (divSize + zTrans);
        if( minPipeLen < 0.0f )
            minPipeLen = 0.0f;

        // Choose length of straight section
        // (we are translating from turnFactor to 'straightFactor' here)
        pipeLen = minPipeLen +
            ss_fRand( 1.0f - turnFactorMax, 1.0f - turnFactorMin ) * 
                        (maxPipeLen - minPipeLen);

        // turn radius is whatever's left over:
        turnRadius = maxPipeLen - pipeLen + minTurnRadius;

        // draw straight section
        DrawExtrudedXCObject( pipeLen );
        zTrans += pipeLen; // not necessary for now, since elbow no use

        // draw elbow
        // this updates axes, notchVec to position at end of elbow
        DrawXCElbow( newDir, turnRadius );

        zTrans = -(divSize - turnRadius);  // distance back from node
    }
    else {  // no turn
        // draw a straight pipe through the current node
        // length can vary according to the turnFactors (e.g. for high turn
        // factors draw a short pipe, so next turn can be as big as possible)

        minPipeLen = -zTrans; // brings us just up to last node
        maxPipeLen = minPipeLen + divSize - xcCur->MaxExtent();
        // brings us as close as possible to new node

        pipeLen = minPipeLen +
            ss_fRand( 1.0f - turnFactorMax, 1.0f - turnFactorMin ) * 
                        (maxPipeLen - minPipeLen);

        // draw pipe
        DrawExtrudedXCObject( pipeLen );
        zTrans += (-divSize + pipeLen);
    }

    UpdateCurrentPosition( newDir );

    lastDir = newDir;
}

/**************************************************************************\
* DrawTurningXCPipe
*
* Draws the pipe using only turns
*
* - Go straight if no turns available
*
* History
*  Aug 10, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
TURNING_FLEX_PIPE::Draw()
{
    float turnRadius;
    int newDir;
    NODE_ARRAY *nodes = pState->nodes;
    float divSize = pState->view.divSize;

    // get new direction

    //mf: pipe may have gotten stuck on Start...(we don't check for this)

    newDir = nodes->ChooseNewTurnDirection( &curPos, lastDir );
    if( newDir == DIR_NONE ) {
        status = PIPE_STUCK;
        DrawCap( END_CAP );
        glPopMatrix();
        return;
    }

    if( newDir == DIR_STRAIGHT ) {
        // No turns available - draw straight section and hope for turns
        //  on next iteration
        DrawExtrudedXCObject( divSize );
        UpdateCurrentPosition( lastDir );
        // ! we have to mark node as taken for this case, since
        // ChooseNewTurnDirection doesn't know whether we're taking the
        // straight option or not
        nodes->NodeVisited( &curPos );
    } else {
        // draw turning pipe

        // since xc is always located right at current node, turn radius
        // stays constant at one node division

        turnRadius = divSize;

        DrawXCElbow( newDir, turnRadius );

        // (zTrans stays at 0)

        // need to update 2 nodes
        UpdateCurrentPosition( lastDir );
        UpdateCurrentPosition( newDir );

        lastDir = newDir;
    }
}

/**************************************************************************\
* DrawXCElbow
*
* Draw elbow from current position through new direction
*
* - Extends current xc around bend
* - Radius of bend is provided - this is distance from xc center to hinge
*   point, along newDir.  e.g. for 'normal pipes', radius=vc->radius
*
* History
*  Jul. 25, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
FLEX_PIPE::DrawXCElbow( int newDir, float radius )
{
    int relDir;  // 'relative' direction of turn
    float length;

    length = (2.0f * PI * radius) / 4.0f; // average length of elbow

    // calc vDiv, texture params based on length
    //mf: I think we should improve resolution of elbows - more vDiv's
    // could rewrite this fn to take a vDivSize
    CalcEvalLengthParams( length );

    pEval->vOrder = EVAL_ARC_ORDER;

    // convert absolute dir to relative dir
    relDir = GetRelativeDir( lastDir, notchVec, newDir );

    // draw it - call simple bend function

    pEval->ProcessXCPrimBendSimple( xcCur, relDir, radius );

    // set transf. matrix to new position by translating/rotating/translating
    // ! Based on simple elbow
    glTranslatef( 0.0f, 0.0f, radius );
    switch( relDir ) {
        case PLUS_X:
            glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
            break;
        case MINUS_X:
            glRotatef( -90.0f, 0.0f, 1.0f, 0.0f );
            break;
        case PLUS_Y:
            glRotatef( -90.0f, 1.0f, 0.0f, 0.0f );
            break;
        case MINUS_Y:
            glRotatef( 90.0f, 1.0f, 0.0f, 0.0f );
            break;
    }
    glTranslatef( 0.0f, 0.0f, radius );
    
    // update notch vector using old function
    notchVec = notchTurn[lastDir][newDir][notchVec];
}

/**************************************************************************\
* DrawExtrudedXCObject
*
* Draws object generated by extruding the current xc
*
* Object starts at xc at origin in z=0 plane, and grows along +z axis 
*
* History
*  July 5, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
FLEX_PIPE::DrawExtrudedXCObject( float length )
{
    // calc vDiv, and texture coord stuff based on length
    // this also calcs pEval texture ctrl pt arrray now
    CalcEvalLengthParams( length );

    // we can fill in some more stuff:
    pEval->vOrder = EVAL_CYLINDER_ORDER;

#if 0
    // continuity stuff
    prim.contStart = prim.contEnd = CONT_1; // geometric continuity
#endif

    // draw it

//mf: this fn doesn't really handle continutity for 2 different xc's, so
// may as well pass it one xc
    pEval->ProcessXCPrimLinear( xcCur, xcCur, length );

    // update state draw axes position
    glTranslatef( 0.0f, 0.0f, length );
}

/**************************************************************************\
* DrawXCCap
*
* Cap the start of the pipe
*
* Needs newDir, so it can orient itself.
* Cap ends at current position with approppriate profile, starts a distance
* 'z' back along newDir.
* Profile is a singularity at start point.
*       
* History
*  July 22, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
FLEX_PIPE::DrawCap( int type )
{
    float radius;
    XC *xc = xcCur;
    BOOL bOpening = (type == START_CAP) ? TRUE : FALSE;
    float length;

    // set radius as average of the bounding box min/max's
    radius = ((xc->xRight - xc->xLeft) + (xc->yTop - xc->yBottom)) / 4.0f;

    length = (2.0f * PI * radius) / 4.0f; // average length of arc

    // calc vDiv, and texture coord stuff based on length
    CalcEvalLengthParams( length );

    // we can fill in some more stuff:
    pEval->vOrder = EVAL_ARC_ORDER;

    // draw it

    pEval->ProcessXCPrimSingularity( xc, radius, bOpening );
}

/**************************************************************************\
* CalcEvalLengthParams 
*
* Calculate pEval values that depend on the length of the extruded object
*
* - calculate vDiv, s_start, s_end, and the texture control net array
*
* History
*  July 13, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
FLEX_PIPE::CalcEvalLengthParams( float length )
{
    pEval->vDiv = (int ) SS_ROUND_UP( length / evalDivSize ); 

    // calc texture start and end coords

    if( bTexture ) {
        GLfloat s_delta;

        // Don't let s_end overflow : it should stay in range (0..1.0)
        if( s_end > 1.0f )
            s_end -= (int) s_end;

        s_start = s_end;
        s_delta = (length / s_length );
        s_end = s_start + s_delta;
        
        // the texture ctrl point array can be calc'd here - it is always
        // a simple 2x2 array for each section
        pEval->SetTextureControlPoints( s_start, s_end, t_start, t_end );
    }
}

/**************************************************************************\
*
* GetRelativeDir 
*
* Calculates relative direction of turn from lastDir, notchVec, newDir
*
* - Use look up table for now.
* - Relative direction is from xy-plane, and can be +x,-x,+y,-y   
* - In current orientation, +z is along lastDir, +x along notchVec
*
* History
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

// this array tells you relative turn
// format: relDir[lastDir][notchVec][newDir]
static int relDir[NUM_DIRS][NUM_DIRS][NUM_DIRS] = {
//      +x      -x      +y      -y      +z      -z (newDir)
// lastDir = +x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Y, MINUS_Y,
        iXX,    iXX,    MINUS_X,PLUS_X, MINUS_Y,PLUS_Y,
        iXX,    iXX,    MINUS_Y,PLUS_Y, PLUS_X, MINUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
// lastDir = -x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,MINUS_Y,PLUS_Y,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Y, MINUS_Y,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
        iXX,    iXX,    MINUS_Y,PLUS_Y, MINUS_X,PLUS_X,
// lastDir = +y
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,
        MINUS_X,PLUS_X, iXX,    iXX,    PLUS_Y, MINUS_Y,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_X, MINUS_X,
        MINUS_Y,PLUS_Y, iXX,    iXX,    MINUS_X,PLUS_X,
// lastDir = -y
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
        MINUS_X,PLUS_X, iXX,    iXX,    MINUS_Y,PLUS_Y,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_X, MINUS_X,
        PLUS_Y, MINUS_Y,iXX,    iXX,    MINUS_X,PLUS_X,

// lastDir = +z
        PLUS_X, MINUS_X,PLUS_Y, MINUS_Y,iXX,    iXX,
        MINUS_X,PLUS_X, MINUS_Y,PLUS_Y, iXX,    iXX,
        MINUS_Y,PLUS_Y, PLUS_X, MINUS_X,iXX,    iXX,
        PLUS_Y, MINUS_Y,MINUS_X,PLUS_X, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
// lastDir = -z
        PLUS_X, MINUS_X,MINUS_Y,PLUS_Y, iXX,    iXX,
        MINUS_X,PLUS_X, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,iXX,    iXX,
        MINUS_Y,PLUS_Y, MINUS_X,PLUS_X, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX
};

static int
GetRelativeDir( int lastDir, int notchVec, int newDir )
{
    return( relDir[lastDir][notchVec][newDir] );
}

