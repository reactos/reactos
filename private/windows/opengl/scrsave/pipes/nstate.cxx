/******************************Module*Header*******************************\
* Module Name: nstate.cxx
*
* NORMAL_STATE
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
#include "nstate.h"
#include "objects.h"
#include "dialog.h"

/******************************Public*Routine******************************\
* NORMAL_STATE constructor
*
* Jul. 95 [marcfo]
*
\**************************************************************************/

NORMAL_STATE::NORMAL_STATE( STATE *pState )
{
    // init joint types from dialog settings

    bCycleJointStyles = 0;

    switch( ulJointType ) {
        case JOINT_ELBOW:
            jointStyle = ELBOWS;
            break;
        case JOINT_BALL:
            jointStyle = BALLS;
            break;
        case JOINT_MIXED:
            jointStyle = EITHER;
            break;
        case JOINT_CYCLE:
            bCycleJointStyles = 1;
            jointStyle = EITHER;
            break;
        default:
            break;
    }

    // Build the objects

    BuildObjects( pState->radius, pState->view.divSize, pState->nSlices,
                  pState->bTexture, &pState->texRep[0] );
}

/******************************Public*Routine******************************\
* NORMAL_STATE destructor
*
* Some of the objects are always created, so don't have to check if they
* exist. Others may be NULL.
\**************************************************************************/

NORMAL_STATE::~NORMAL_STATE( )
{
    delete shortPipe;
    delete longPipe;
    delete ballCap;

    for( int i = 0; i < 4; i ++ ) {
        delete elbows[i];
        if( ballJoints[i] )
            delete ballJoints[i];
    }

    if( bigBall )
        delete bigBall;
}



/**************************************************************************\
* BuildObjects
*
* - Build all the pipe primitives
* - Different prims are built based on bTexture flag
*
\**************************************************************************/
void 
NORMAL_STATE::BuildObjects( float radius, float divSize, int nSlices, 
                            BOOL bTexture, IPOINT2D *texRep )
{
    OBJECT_BUILD_INFO *pBuildInfo = new OBJECT_BUILD_INFO;
    pBuildInfo->radius = radius;
    pBuildInfo->divSize = divSize;
    pBuildInfo->nSlices = nSlices;
    pBuildInfo->bTexture = bTexture;

    if( bTexture ) {
        pBuildInfo->texRep = texRep;
        
        // Calc s texture intersection values
        float s_max = (float) texRep->y;
        float s_trans =  s_max * 2.0f * radius / divSize;

        // Build short and long pipes
        shortPipe = new PIPE_OBJECT( pBuildInfo, divSize - 2*radius,
                                     s_trans, s_max );
        longPipe = new PIPE_OBJECT( pBuildInfo, divSize, 0.0f, s_max );

        // Build elbow and ball joints
        for( int i = 0; i < 4; i ++ ) {
            elbows[i] = new ELBOW_OBJECT( pBuildInfo, i, 0.0f, s_trans );
            ballJoints[i] = new BALLJOINT_OBJECT( pBuildInfo, i, 0.0f, s_trans );
        }

        bigBall = NULL;

        // Build end cap

        float s_start = - texRep->x * (ROOT_TWO - 1.0f) * radius / divSize;
        float s_end = texRep->x * (2.0f + (ROOT_TWO - 1.0f)) * radius / divSize;
        // calc compensation value, to prevent negative s coords
        float comp_s = (int) ( - s_start ) + 1.0f;
        s_start += comp_s;
        s_end += comp_s;
        ballCap = new SPHERE_OBJECT( pBuildInfo, ROOT_TWO*radius, s_start, s_end );

    } else {
        // Build pipes, elbows
        shortPipe = new PIPE_OBJECT( pBuildInfo, divSize - 2*radius );
        longPipe = new PIPE_OBJECT( pBuildInfo, divSize );
        for( int i = 0; i < 4; i ++ ) {
            elbows[i] = new ELBOW_OBJECT( pBuildInfo, i );
            ballJoints[i] = NULL;
        }

        // Build just one ball joint when not texturing.  It is slightly
        // larger than standard ball joint, to prevent any pipe edges from
        // 'sticking' out of the ball.
        bigBall = new SPHERE_OBJECT( pBuildInfo,  
                     ROOT_TWO*radius / ((float) cos(PI/nSlices)) );

        // build end cap
        ballCap = new SPHERE_OBJECT( pBuildInfo, ROOT_TWO*radius );
    }
}

/**************************************************************************\
* Reset
*
* Reset frame attributes for normal pipes.
*
\**************************************************************************/

void 
NORMAL_STATE::Reset( )
{
    // Set the joint style
    if( bCycleJointStyles ) {
        if( ++(jointStyle) >= NUM_JOINT_STYLES )
            jointStyle = 0;
    }
}

#if 0
/**************************************************************************\
* GetMaxPipesPerFrame
*
\**************************************************************************/

int
NORMAL_STATE::GetMaxPipesPerFrame( )
{
    if( bTexture )
        return NORMAL_TEX_PIPE_COUNT;
    else
        return NORMAL_PIPE_COUNT;
}
#endif

/*-----------------------------------------------------------------------
|                                                                       |
|    ChooseJointType                                                    |
|       - Decides which type of joint to draw                           |
|                                                                       |
-----------------------------------------------------------------------*/

#define BLUE_MOON 153

int 
NORMAL_STATE::ChooseJointType( )
{
    switch( jointStyle ) {
        case ELBOWS:
            return ELBOW_JOINT;
        case BALLS:
            return BALL_JOINT;
        case EITHER:
            // draw a teapot once in a blue moon
            if( ss_iRand(1000) == BLUE_MOON )
                return( TEAPOT );
        default:
            // otherwise an elbow or a ball (1/3 ball)
            if( !ss_iRand(3) )
                return BALL_JOINT;
            else
                return ELBOW_JOINT;
    }
}

