/******************************Module*Header*******************************\
* Module Name: npipe.cxx
*
* Normal pipes code
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>

#include "sspipes.h"
#include "npipe.h"
#include "state.h"


static void align_notch( int newDir, int notch );
static void align_plusy( int oldDir, int newDir );

// defCylNotch shows where the notch for the default cylinder will be,
//  in absolute coords, once we do an align_plusz

static GLint defCylNotch[NUM_DIRS] = 
        { PLUS_Y, PLUS_Y, MINUS_Z, PLUS_Z, PLUS_Y, PLUS_Y };


/**************************************************************************\
* NORMAL_PIPE constructor
*
*
\**************************************************************************/

NORMAL_PIPE::NORMAL_PIPE( STATE *pState )
: PIPE( pState )
{
    int choice;

    type = TYPE_NORMAL;
    pNState = pState->pNState;

    // choose weighting of going straight
    if( ! ss_iRand( 20 ) )
        weightStraight = ss_iRand2( MAX_WEIGHT_STRAIGHT/4, MAX_WEIGHT_STRAIGHT );
    else
        weightStraight = 1 + ss_iRand( 4 );
}

/**************************************************************************\
* Start
*
* Start drawing a new normal pipe
*
* - Draw a start cap and short pipe in new direction
*
* History
*  July 27, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
NORMAL_PIPE::Start( )
{
    int newDir;

    // Set start position

    if( !SetStartPos() ) {
        status = PIPE_OUT_OF_NODES;
        return;
    }

    // set a material

    ChooseMaterial();

    // push matrix that has initial zTrans and rotation
    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // Pick a random lastDir
    lastDir = ss_iRand( NUM_DIRS );

    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) {
        // pipe is stuck at the start node, draw something
        status = PIPE_STUCK;
        DrawTeapot();
        glPopMatrix();
        return;
    } else
        status = PIPE_ACTIVE;

    // set initial notch vector
    notchVec = defCylNotch[newDir];

    DrawStartCap( newDir );

    // move ahead 1.0*r to draw pipe
    glTranslatef( 0.0f, 0.0f, radius );
            
    // draw short pipe
    align_notch( newDir, notchVec );
    pNState->shortPipe->Draw();

    glPopMatrix();

    UpdateCurrentPosition( newDir );

    lastDir = newDir;
}

/**************************************************************************\
* Draw
*
* - if turning, draws a joint and a short cylinder, otherwise
*   draws a long cylinder.
* - the 'current node' is set as the one we draw thru the NEXT
*   time around.
*
\**************************************************************************/

void
NORMAL_PIPE::Draw()
{
    int newDir;

    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) {  // no empty nodes - nowhere to go
        DrawEndCap();
        status = PIPE_STUCK;
        return;
    }

    // push matrix that has initial zTrans and rotation
    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // draw joint if necessary, and pipe

    if( newDir != lastDir ) { // turning! - we have to draw joint
        DrawJoint( newDir );

        // draw short pipe
        align_notch( newDir, notchVec );
        pNState->shortPipe->Draw();
    }
    else {  // no turn
        // draw long pipe, from point 1.0*r back
        align_plusz( newDir );
        align_notch( newDir, notchVec );
        glTranslatef( 0.0f, 0.0f, -radius );
        pNState->longPipe->Draw();
    }

    glPopMatrix();

    UpdateCurrentPosition( newDir );

    lastDir = newDir;
}

/**************************************************************************\
* DrawStartCap
*
* Cap the start of the pipe with a ball
*
* History
*  July 4, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void 
NORMAL_PIPE::DrawStartCap( int newDir )
{
    if( bTexture ) {
        align_plusz( newDir );
        pNState->ballCap->Draw();
    }
    else {
        // draw big ball in default orientation
        pNState->bigBall->Draw();
        align_plusz( newDir );
    }
}

/**************************************************************************\
* DrawEndCap():
*
* - Draws a ball, used to cap end of a pipe
*
\**************************************************************************/

void 
NORMAL_PIPE::DrawEndCap( )
{
    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    if( bTexture ) {
        glPushMatrix();
        align_plusz( lastDir );
        align_notch( lastDir, notchVec );
        pNState->ballCap->Draw();
        glPopMatrix();
    }
    else
        pNState->bigBall->Draw();

    glPopMatrix();
}

/**************************************************************************\
* ChooseElbow
*
* - Decides which elbow to draw
* - The beginning of each elbow is aligned along +y, and we have
*   to choose the one with the notch in correct position
* - The 'primary' start notch (elbow[0]) is in same direction as
*   newDir, and successive elbows rotate this notch CCW around +y
*
\**************************************************************************/


// this array supplies the sequence of elbow notch vectors, given
//  oldDir and newDir  (0's are don't cares)
// it is also used to determine the ending notch of an elbow
static GLint notchElbDir[NUM_DIRS][NUM_DIRS][4] = {
// oldDir = +x
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Y,         MINUS_Z,        MINUS_Y,        PLUS_Z,
        MINUS_Y,        PLUS_Z,         PLUS_Y,         MINUS_Z,
        PLUS_Z,         PLUS_Y,         MINUS_Z,        MINUS_Y,
        MINUS_Z,        MINUS_Y,        PLUS_Z,         PLUS_Y,
// oldDir = -x
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Y,         PLUS_Z,         MINUS_Y,        MINUS_Z,
        MINUS_Y,        MINUS_Z,        PLUS_Y,         PLUS_Z,
        PLUS_Z,         MINUS_Y,        MINUS_Z,        PLUS_Y,
        MINUS_Z,        PLUS_Y,         PLUS_Z,         MINUS_Y,
// oldDir = +y
        PLUS_X,         PLUS_Z,         MINUS_X,        MINUS_Z,
        MINUS_X,        MINUS_Z,        PLUS_X,         PLUS_Z,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Z,         MINUS_X,        MINUS_Z,        PLUS_X,
        MINUS_Z,        PLUS_X,         PLUS_Z,         MINUS_X,
// oldDir = -y
        PLUS_X,         MINUS_Z,        MINUS_X,        PLUS_Z,
        MINUS_X,        PLUS_Z,         PLUS_X,         MINUS_Z,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Z,         PLUS_X,         MINUS_Z,        MINUS_X,
        MINUS_Z,        MINUS_X,        PLUS_Z,         PLUS_X,
// oldDir = +z
        PLUS_X,         MINUS_Y,        MINUS_X,        PLUS_Y,
        MINUS_X,        PLUS_Y,         PLUS_X,         MINUS_Y,
        PLUS_Y,         PLUS_X,         MINUS_Y,        MINUS_X,
        MINUS_Y,        MINUS_X,        PLUS_Y,         PLUS_X,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
// oldDir = -z
        PLUS_X,         PLUS_Y,         MINUS_X,        MINUS_Y,
        MINUS_X,        MINUS_Y,        PLUS_X,         PLUS_Y,
        PLUS_Y,         MINUS_X,        MINUS_Y,        PLUS_X,
        MINUS_Y,        PLUS_X,         PLUS_Y,         MINUS_X,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX
};

GLint 
NORMAL_PIPE::ChooseElbow( int oldDir, int newDir )
{
    int i;

    // precomputed table supplies correct elbow orientation
    for( i = 0; i < 4; i ++ ) {
        if( notchElbDir[oldDir][newDir][i] == notchVec )
            return i;
    }
    // we shouldn't arrive here
    return -1;
}

/**************************************************************************\
* DrawJoint
*
* Draw a joint between 2 pipes
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

void
NORMAL_PIPE::DrawJoint( int newDir )
{
    int jointType;
    int iBend;

    jointType = pNState->ChooseJointType();
#if PIPES_DEBUG
    if( newDir == oppositeDir[lastDir] )
        printf( "Warning: opposite dir chosen!\n" );
#endif
        
    switch( jointType ) {
      case BALL_JOINT:
            if( bTexture ) {
                // use special texture-friendly ballJoints

                align_plusz( newDir );
                glPushMatrix();

                align_plusy( lastDir, newDir );

                // translate forward 1.0*r along +z to get set for drawing elbow
                glTranslatef( 0.0f, 0.0f, radius );
                // decide which elbow orientation to use
                iBend = ChooseElbow( lastDir, newDir );
                pNState->ballJoints[iBend]->Draw();

                glPopMatrix();
            }
            else {
                // draw big ball in default orientation
                pNState->bigBall->Draw();
                align_plusz( newDir );
            }
            // move ahead 1.0*r to draw pipe
            glTranslatef( 0.0f, 0.0f, radius );
        break;

      case ELBOW_JOINT:
            align_plusz( newDir );

            // the align_plusy() here will screw up our notch calcs, so
            //  we push-pop

            glPushMatrix();

            align_plusy( lastDir, newDir );

            // translate forward 1.0*r along +z to get set for drawing elbow
            glTranslatef( 0.0f, 0.0f, radius );
            // decide which elbow orientation to use
            iBend = ChooseElbow( lastDir, newDir );
            if( iBend == -1 ) {
#if PIPES_DEBUG
                printf( "ChooseElbow() screwed up\n" );
#endif
                iBend = 0; // recover
            }
            pNState->elbows[iBend]->Draw();

            glPopMatrix();

            glTranslatef( 0.0f, 0.0f, radius );
        break;

      default:
            // Horrors! It's the teapot!
            DrawTeapot();
            align_plusz( newDir );
            // move ahead 1.0*r to draw pipe
            glTranslatef( 0.0f, 0.0f, radius );
        }
            
        // update the current notch vector
        notchVec = notchTurn[lastDir][newDir][notchVec];
#if PIPES_DEBUG
        if( notchVec == iXX )
            printf( "notchTurn gave bad value\n" );
#endif
}


/**************************************************************************\
* Geometry functions
\**************************************************************************/


static float RotZ[NUM_DIRS][NUM_DIRS] = {
          0.0f,   0.0f,  90.0f,  90.0f,  90.0f, -90.0f,
          0.0f,   0.0f, -90.0f, -90.0f, -90.0f,  90.0f,
        180.0f, 180.0f,   0.0f,   0.0f, 180.0f, 180.0f,
          0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,
        -90.0f,  90.0f,   0.0f, 180.0f,   0.0f,   0.0f,
         90.0f, -90.0f, 180.0f,   0.0f,   0.0f,   0.0f };

        
            
/*-----------------------------------------------------------------------
|                                                                       |
|    align_plusy( int lastDir, int newDir )                             |
|       - Assuming +z axis is already aligned with newDir, align        |
|         +y axis BACK along lastDir                                    |
|                                                                       |
-----------------------------------------------------------------------*/

static void 
align_plusy( int oldDir, int newDir )
{
    GLfloat rotz;

    rotz = RotZ[oldDir][newDir];
    glRotatef( rotz, 0.0f, 0.0f, 1.0f );
}

// given a dir, determine how much to rotate cylinder around z to match notches
// format is [newDir][notchVec]

static GLfloat alignNotchRot[NUM_DIRS][NUM_DIRS] = {
        fXX,    fXX,    0.0f,   180.0f,  90.0f, -90.0f,
        fXX,    fXX,    0.0f,   180.0f,  -90.0f, 90.0f,
        -90.0f, 90.0f,  fXX,    fXX,    180.0f, 0.0f,
        -90.0f, 90.0f,  fXX,    fXX,    0.0f,   180.0f,
        -90.0f, 90.0f,  0.0f,   180.0f, fXX,    fXX,
        90.0f,  -90.0f, 0.0f,   180.0f, fXX,    fXX
};
                
                
/*-----------------------------------------------------------------------
|                                                                       |
|    align_notch( int newDir )                                          |
|       - a cylinder is notched, and we have to line this up            |
|         with the previous primitive's notch which is maintained as    |
|         notchVec.                                                     |
|       - this adds a rotation around z to achieve this                 |
|                                                                       |
-----------------------------------------------------------------------*/

static void 
align_notch( int newDir, int notch )
{
    GLfloat rotz;
    GLint curNotch;

    // figure out where notch is presently after +z alignment
    curNotch = defCylNotch[newDir];
    // (don't need this now we have lut)

    // look up rotation value in table
    rotz = alignNotchRot[newDir][notch];
#if PIPES_DEBUG
    if( rotz == fXX ) {
        printf( "align_notch(): unexpected value\n" );
        return;
    }
#endif

    if( rotz != 0.0f )
        glRotatef( rotz, 0.0f, 0.0f, 1.0f );
}
