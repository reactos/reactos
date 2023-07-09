/******************************Module*Header*******************************\
* Module Name: view.h
*
* Node stuff
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __view_h__
#define __view_h__

#include "sscommon.h"

typedef struct {
    float viewAngle;            // field of view angle for height
    float zNear;                // near z clip value
    float zFar;                 // far z clip value
} Perspective;  // perspective view description

class VIEW {
public:
    float       zTrans;         // z translation
    float       yRot;           // current yRotation
    float       viewDist;       // viewing distance, usually -zTrans
    int         numDiv;         // # grid divisions in x,y,z
    float       divSize;        // distance between divisions
    ISIZE       winSize;        // window size in pixels

    VIEW();
    BOOL        SetWinSize( int width, int height );
    void        SetGLView();
    void        CalcNodeArraySize( IPOINT3D *pNodeDim );
    void        SetProjMatrix();
    void        IncrementSceneRotation();
private:
    BOOL        bProjMode;      // projection mode
    Perspective persp;          // perspective view description
    float       aspectRatio;    // x/y window aspect ratio
    POINT3D     world;          // view area in world space
};

#endif // __view_h__
