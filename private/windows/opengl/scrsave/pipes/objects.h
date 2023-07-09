/******************************Module*Header*******************************\
* Module Name: objects.h
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef __objects_h__
#define __objects_h__

#include "sspipes.h"
#include "state.h"

class OBJECT_BUILD_INFO {
public:
    float radius;
    float divSize;
    int   nSlices;
    BOOL  bTexture;
    IPOINT2D *texRep;
};

/**************************************************************************\
*
* OBJECT classes
*
* - Display list objects
*
\**************************************************************************/

class OBJECT {
protected:
    int         listNum;
    int         nSlices;
public:
    void        Draw();

    OBJECT();
    ~OBJECT();
};

class PIPE_OBJECT : public OBJECT {
private:
    void Build( OBJECT_BUILD_INFO *state, float length, float start_s, float s_end );
public:
    PIPE_OBJECT( OBJECT_BUILD_INFO *state, float length );
    PIPE_OBJECT( OBJECT_BUILD_INFO *state, float length, float start_s, float end_s );
};

class ELBOW_OBJECT : public OBJECT {
private:
    void Build( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
public:
    ELBOW_OBJECT( OBJECT_BUILD_INFO *state, int notch );
    ELBOW_OBJECT( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
};

class BALLJOINT_OBJECT : public OBJECT {
private:
    void Build( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
public:
    // texturing version only
    BALLJOINT_OBJECT( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
};

class SPHERE_OBJECT : public OBJECT {
private:
    void Build( OBJECT_BUILD_INFO *state, float radius, float start_s, float end_s );
public:
    SPHERE_OBJECT( OBJECT_BUILD_INFO *state, float radius, float start_s, float end_s );
    SPHERE_OBJECT( OBJECT_BUILD_INFO *state, float radius );
};

#endif // __objects_h__
