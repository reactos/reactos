/******************************Module*Header*******************************\
* Module Name: sspipes.h
*
* Global header for 3D Pipes screen saver.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef __sspipes_h__
#define __sspipes_h__

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glaux.h>

#include <commctrl.h>

#include "sscommon.h"

#define DO_TIMING 1
#ifdef DO_TIMING
enum {
    TIMER_START = 0,
    TIMER_STOP,
    TIMER_TIMING,
    TIMER_RESET
};
extern void Timer( int mode );
extern int pipeCount;
#endif


#define iXX -1
#define fXX -0.01f

// These are absolute directions, with origin in center of screen,
// looking down -z

enum {
    PLUS_X = 0,
    MINUS_X,
    PLUS_Y,
    MINUS_Y,
    PLUS_Z,
    MINUS_Z,
    NUM_DIRS,
    DIR_NONE,
    DIR_STRAIGHT
};

#define NUM_DIV 16              // divisions in window in longest dimension

#define MAX_TEXTURES 8

#endif // __sspipes_h__
