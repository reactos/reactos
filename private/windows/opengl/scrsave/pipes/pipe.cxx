/******************************Module*Header*******************************\
* Module Name: pipe.cxx
*
* - Pipe base class stuff
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
#include "state.h"
#include "pipe.h"

/******************************Public*Routine******************************\
* PIPE constructor
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

PIPE::PIPE( STATE *state )
{
    pState = state;
    bTexture = pState->bTexture;
    radius = pState->radius;
    // default direction choosing is random
    chooseDirMethod = CHOOSE_DIR_RANDOM_WEIGHTED;
    chooseStartPosMethod = CHOOSE_STARTPOS_RANDOM;
    weightStraight = 1;
}

/******************************Public*Routine******************************\
* ChooseMaterial
*
\**************************************************************************/

void 
PIPE::ChooseMaterial( )
{
    if( bTexture )
        ss_RandomTexMaterial( TRUE );
    else
        ss_RandomTeaMaterial( TRUE );
}

/**************************************************************************\
*
* DrawTeapot
*
\**************************************************************************/

extern void ResetEvaluator( BOOL bTexture );

void 
PIPE::DrawTeapot( )
{
    glFrontFace( GL_CW );
    glEnable( GL_NORMALIZE );
    auxSolidTeapot(2.5 * radius);
    glDisable( GL_NORMALIZE );
    glFrontFace( GL_CCW );
    if( type != TYPE_NORMAL ) {
        // Re-init flex's evaluator state (teapot uses evaluators as well,
        //  and messes up the state).
        ResetEvaluator( bTexture );
    }
}

/******************************Public*Routine******************************\
* SetChooseDirectionMethod
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

void
PIPE::SetChooseDirectionMethod( int method )
{
    chooseDirMethod = method;
}


/**************************************************************************\
*
* ChooseNewDirection
*
* Call direction-finding function based on current method
* This is a generic entry point that is used by some pipe types
*
\**************************************************************************/

int 
PIPE::ChooseNewDirection()
{
    NODE_ARRAY *nodes = pState->nodes;
    int bestDirs[NUM_DIRS], nBestDirs;

    // figger out which fn to call
    switch( chooseDirMethod ) {
        case CHOOSE_DIR_CHASE:
            if( nBestDirs = GetBestDirsForChase( bestDirs ) )
                return nodes->ChoosePreferredDirection( &curPos, lastDir, 
                                                        bestDirs, nBestDirs );
            // else lead pipe must have died, so fall thru:
        case CHOOSE_DIR_RANDOM_WEIGHTED :
        default:
            return nodes->ChooseRandomDirection( &curPos, lastDir, weightStraight );
    }
}

/**************************************************************************\
*
* GetBestDirsForChase
*
* Find the best directions to take to close in on the lead pipe in chase mode.
*
\**************************************************************************/

//mf: ? but want to use similar scheme for turning flex pipes !!
// (later) 
int
PIPE::GetBestDirsForChase( int *bestDirs )
{
    // Figure out best dirs to close in on leadPos

    //mf: will have to 'protect' leadPos with GetLeadPos() for multi-threading
    IPOINT3D *leadPos = &pState->pLeadPipe->curPos;
    IPOINT3D delta;
    int numDirs = 0;

    delta.x = leadPos->x - curPos.x;
    delta.y = leadPos->y - curPos.y;
    delta.z = leadPos->z - curPos.z;

    if( delta.x ) {
        numDirs++;
        *bestDirs++ = delta.x > 0 ? PLUS_X : MINUS_X;
    }
    if( delta.y ) {
        numDirs++;
        *bestDirs++ = delta.y > 0 ? PLUS_Y : MINUS_Y;
    }
    if( delta.z ) {
        numDirs++;
        *bestDirs++ = delta.z > 0 ? PLUS_Z : MINUS_Z;
    }
    // It should be impossible for numDirs = 0 (all deltas = 0), as this
    // means curPos = leadPos
    return numDirs;
}

/******************************Public*Routine******************************\
* SetChooseStartPosMethod
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

void
PIPE::SetChooseStartPosMethod( int method )
{
    chooseStartPosMethod = method;
}

/******************************Public*Routine******************************\
* PIPE::SetStartPos
*
* - Find an empty node to start the pipe on
*
\**************************************************************************/

BOOL
PIPE::SetStartPos()
{
    NODE_ARRAY *nodes = pState->nodes;

    switch( chooseStartPosMethod ) {

        case CHOOSE_STARTPOS_RANDOM:
        default:
            if( !nodes->FindRandomEmptyNode( &curPos ) ) {
                return FALSE;
            }
            return TRUE;
        
        case CHOOSE_STARTPOS_FURTHEST:
            // find node furthest away from curPos
            IPOINT3D refPos, numNodes;
            nodes->GetNodeCount( &numNodes );
            refPos.x = (curPos.x >= (numNodes.x / 2)) ? 0 : numNodes.x - 1;
            refPos.y = (curPos.y >= (numNodes.y / 2)) ? 0 : numNodes.y - 1;
            refPos.z = (curPos.z >= (numNodes.z / 2)) ? 0 : numNodes.z - 1;

            if( !nodes->TakeClosestEmptyNode( &curPos, &refPos ) ) {
                return FALSE;
            }
            return TRUE;
    }
}

/******************************Public*Routine******************************\
* PIPE::IsStuck
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

BOOL
PIPE::IsStuck()
{
    return status == PIPE_STUCK;
}

/******************************Public*Routine******************************\
* PIPE::TranslateToCurrentPosition
*
\**************************************************************************/

void
PIPE::TranslateToCurrentPosition()
{
    IPOINT3D numNodes;

    float divSize = pState->view.divSize;
    // this requires knowing the size of the node array
    pState->nodes->GetNodeCount( &numNodes );
    glTranslatef( (curPos.x - (numNodes.x - 1)/2.0f )*divSize,
                  (curPos.y - (numNodes.y - 1)/2.0f )*divSize,
                  (curPos.z - (numNodes.z - 1)/2.0f )*divSize );
}

/**************************************************************************\
*
* UpdateCurrentPosition
* 
* Increment current position according to direction taken
\**************************************************************************/

void 
PIPE::UpdateCurrentPosition( int newDir )
{
    switch( newDir ) {
        case PLUS_X:
            curPos.x += 1;
            break;
        case MINUS_X:
            curPos.x -= 1;
            break;
        case PLUS_Y:
            curPos.y += 1;
            break;
        case MINUS_Y:
            curPos.y -= 1;
            break;
        case PLUS_Z:
            curPos.z += 1;
            break;
        case MINUS_Z:
            curPos.z -= 1;
            break;
    }
}

/******************************Public*Routine******************************\
* align_plusz
*
* - Aligns the z axis along specified direction
* - Used for all types of pipes
*
\**************************************************************************/


void align_plusz( int newDir )
{
    // align +z along new direction
    switch( newDir ) {
        case PLUS_X:
            glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
            break;
        case MINUS_X:
            glRotatef( -90.0f, 0.0f, 1.0f, 0.0f);
            break;
        case PLUS_Y:
            glRotatef( -90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case MINUS_Y:
            glRotatef( 90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case PLUS_Z:
            glRotatef( 0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case MINUS_Z:
            glRotatef( 180.0f, 0.0f, 1.0f, 0.0f);
            break;
    }

}

/**************************************************************************\
* this array tells you which way the notch will be once you make
* a turn
* format: notchTurn[oldDir][newDir][notchVec] 
*
\**************************************************************************/

GLint notchTurn[NUM_DIRS][NUM_DIRS][NUM_DIRS] = {
// oldDir = +x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
// oldDir = -x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
// oldDir = +y
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_Z, MINUS_Z,
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_Z, MINUS_Z,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
// oldDir = -y
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_Z, MINUS_Z,
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_Z, MINUS_Z,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,
// oldDir = +z
        MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,    iXX,
        PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
// oldDir = -z
        PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,    iXX,
        MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,    iXX,
        PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX
};
