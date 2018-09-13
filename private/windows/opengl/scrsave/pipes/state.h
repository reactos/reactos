/******************************Module*Header*******************************\
* Module Name: state.h
*
* STATE
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __state_h__
#define __state_h__

#include "sscommon.hxx"
#include "pipe.h"
#include "node.h"
#include "view.h"
#include "nstate.h"
#include "fstate.h"

#define MAX_DRAW_THREADS    4

#define TEAPOT             66

#define MAX_TESS 3

// type(s) of pipes that are drawn
enum {
    DRAW_NORMAL,
    DRAW_FLEX,
    DRAW_BOTH  // not currently used
};

// Reset status

#define  RESET_STARTUP_BIT (1L << 0)
#define  RESET_NORMAL_BIT  (1L << 1)
#define  RESET_RESIZE_BIT  (1L << 2)
#define  RESET_REPAINT_BIT  (1L << 3)

// Frame draw schemes

enum {
    FRAME_SCHEME_RANDOM,  // pipes draw randomly
    FRAME_SCHEME_CHASE,   // pipes chase a lead pipe
};

class DRAW_THREAD {
private:
    HDC         hdc;
    HTEXTURE    htex;

public:
    HGLRC       hglrc;        // rc to draw with (public so STATE can delete)
    int         priority;

    DRAW_THREAD();
    ~DRAW_THREAD();
    PIPE        *pPipe;       // generic pipe ptr
    void        SetRCDC( HGLRC rc, HDC hdc );
    BOOL        HasRC();
    HGLRC       GetRC();
    void        MakeRCCurrent();
    void        SetTexture( HTEXTURE htex );
    void        SetPipe( PIPE *pipe );
    BOOL        StartPipe();
    void        DrawPipe();
    void        KillPipe();
};

// Program existence instance

class NORMAL_STATE;
class FLEX_STATE;

class STATE {
public:
    HGLRC       shareRC;        // RC that objects are shared from

    PIPE        *pLeadPipe;     // lead pipe for chase scenarios

    int         nSlices;      // reference # of slices around a pipe
    BOOL        bTexture;       // global texture enable
    int         nTextures;
    TEXTURE     texture[MAX_TEXTURES];
    IPOINT2D    texRep[MAX_TEXTURES];

    VIEW        view;           // viewing parameters
    float       radius;         // 'reference' pipe radius value
    NODE_ARRAY  *nodes;         // for keeping track of draw space
    NORMAL_STATE *pNState;
    FLEX_STATE  *pFState;

    STATE( BOOL bFlexMode, BOOL bMultiPipes );
    ~STATE();
    void        Reshape( int width, int height, void *data );
    void        Repaint( LPRECT pRect, void *data );
    void        Draw( void *data );
    void        Finish( void *data );

private:
    int         drawMode;       // drawing mode (flex or normal for now)
    int         drawScheme;     // random or chase

    int         maxPipesPerFrame; // max number of separate pipes/frame
    int         nPipesDrawn;    // number of pipes drawn or drawing in frame
    int         maxDrawThreads; // max number of concurrently drawing pipes
    int         nDrawThreads;   // number of live threads
    DRAW_THREAD drawThreads[MAX_DRAW_THREADS];

    int         resetStatus;

    SS_DIGITAL_DISSOLVE_CLEAR ddClear;
    int         bCalibrateClear;

    void        GLInit();
    void        DrawValidate();  // validation to do before each Draw
    void        ResetView();
    void        FrameReset();
    void        Clear();
    void        ChooseNewLeadPipe();
    void        CompactThreadList();
    BOOL        LoadTextureFiles();
    BOOL        LoadTextureFiles( TEXFILE *pTexFile, int nTexFiles, 
                                  TEX_RES *pTexRes );
    void        CalcTexRepFactors();
    int         CalcMaxPipesPerFrame();
};

#endif // __state_h__
