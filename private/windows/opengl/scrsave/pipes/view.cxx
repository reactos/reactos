/******************************Module*Header*******************************\
* Module Name: view.cxx
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
#include "view.h"

/******************************Public*Routine******************************\
* VIEW constructor
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

VIEW::VIEW()
{
    bProjMode = GL_TRUE;

    // set some initial viewing and size params

//mf: these should be defines
    zTrans = -75.0f;
    viewDist = -zTrans;

    numDiv = NUM_DIV;
    SS_ASSERT( numDiv >= 2, "VIEW constructor: not enough divisions\n" );
    // Because number of nodes in a dimension is derived from (numDiv-1), and
    // can't be 0

    divSize = 7.0f;

    persp.viewAngle = 90.0f;
    persp.zNear = 1.0f;

    yRot = 0.0f;

    winSize.width = winSize.height = 0; 
}

/******************************Public*Routine******************************\
* SetProjMatrix
*
* Set GL view parameters
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

void
VIEW::SetGLView()
{
    glViewport(0, 0, winSize.width, winSize.height );
    SetProjMatrix();
}

/******************************Public*Routine******************************\
* SetProjMatrix
*
* Set Projection matrix
*
* Nov. 95 [marcfo]
*
\**************************************************************************/

void
VIEW::SetProjMatrix()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    persp.zFar = viewDist + world.z*2;
    if( bProjMode ) {
        gluPerspective( persp.viewAngle, 
                        aspectRatio, 
                        persp.zNear, persp.zFar );
    }
    else {
        glOrtho( -world.x/2, world.x/2, -world.y/2, world.y/2,
                          -world.z, world.z );
    }
    glMatrixMode(GL_MODELVIEW);
}

/******************************Public*Routine******************************\
* CalcNodeArraySize
*
* Based on the viewing width and height, and numDiv, calculate the x,y,z array
* node dimensions.
*
\**************************************************************************/

void
VIEW::CalcNodeArraySize( IPOINT3D *pNodeDim )
{
    // mf: !!! if aspect ratio deviates too much from 1, then nodes will get
    // clipped as view rotates

    if( winSize.width >= winSize.height ) {
        pNodeDim->x = numDiv - 1;
        pNodeDim->y = (int) (pNodeDim->x / aspectRatio) ;
        if( pNodeDim->y < 1 )
            pNodeDim->y = 1;
        pNodeDim->z = pNodeDim->x;
    }
    else {
        pNodeDim->y = numDiv - 1;
        pNodeDim->x = (int) (aspectRatio * pNodeDim->y);
        if( pNodeDim->x < 1 )
            pNodeDim->x = 1;
        pNodeDim->z = pNodeDim->y;
    }
}

/******************************Public*Routine******************************\
* SetWinSize
*
* Set the window size for the view, derive other view params.
*
* Return FALSE if new size same as old.
\**************************************************************************/

BOOL
VIEW::SetWinSize( int width, int height )
{
    if( (width == winSize.width) &&
        (height == winSize.height) )
        return FALSE;

    winSize.width = width;
    winSize.height = height;

    aspectRatio = winSize.height == 0 ? 1.0f : (float)winSize.width/winSize.height;

    if( winSize.width >= winSize.height ) {
        world.x = numDiv * divSize;
        world.y = world.x / aspectRatio;
        world.z = world.x;
    }
    else {
        world.y = numDiv * divSize;
        world.x = world.y * aspectRatio;
        world.z = world.y;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
* SetSceneRotation 
*
\**************************************************************************/

void
VIEW::IncrementSceneRotation()
{
    yRot += 9.73156f;
    if( yRot >= 360.0f )
        // prevent overflow
        yRot -= 360.0f;
}
