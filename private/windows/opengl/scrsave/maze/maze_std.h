#ifndef __MAZE_STD_H__
#define __MAZE_STD_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fixed.h"
#include "genmaze.h"
#include "sscommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IntPt2
{
    int x, y;
} IntPt2;

#define DIR_RIGHT 0
#define DIR_UP 1
#define DIR_LEFT 2
#define DIR_DOWN 3
#define DIR_COUNT 4

typedef struct
{
    TEXTURE  *pTex;   // ptr to texture
    IPOINT2D texRep;  // texture repetition in s and t directions
    BOOL     bPalRot; // whether should rotate texture palette
    int      iPalRot; // current palette rotation start point
    BOOL     bTransp; // transparency on/off for RGBA textures
} TEX_ENV;

typedef struct _MazeView
{
    FxPt2 pos;
    FaAngle ang;
} MazeView;

#define RENDER_NONE     0
#define RENDER_FLAT     1
#define RENDER_SMOOTH   2
#define RENDER_TEXTURED 3
#define RENDER_COUNT    4

// surfaces
enum {
    WALLS = 0,
    FLOOR,
    CEILING,
    NUM_SURFACES
};

// default surface textures
enum {
    BRICK_TEXTURE = 0,
    WOOD_TEXTURE,
    CASTLE_TEXTURE,
    CURL4_TEXTURE,
    BHOLE4_TEXTURE,
    SNOWFLAK_TEXTURE,
    SWIRLX4_TEXTURE,
    NUM_DEF_SURFACE_TEXTURES
};

// textured objects
enum {
    START = 0,
    END,
    RAT,
    AD,
    COVER,
    NUM_OBJECT_TEXTURES
};


enum {
    IMAGEQUAL_DEFAULT = 0,
    IMAGEQUAL_HIGH,
    IMAGEQUAL_COUNT
};

typedef struct _MazeOptions
{
    BOOL depth_test;
    int render[NUM_SURFACES];
    BOOL frame_count;
    BOOL top_view;
    BOOL eye_view;
    BOOL single_step;
    BOOL all_alpha;
    BOOL bDither;
    int nrats;
} MazeOptions;

extern MazeOptions maze_options;

#ifdef __cplusplus
}
#endif

#endif // __MAZE_STD_H__
