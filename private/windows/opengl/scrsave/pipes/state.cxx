/******************************Module*Header*******************************\
* Module Name: state.cxx
*
* STATE
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
#include "dialog.h"
#include "state.h"
#include "pipe.h"
#include "npipe.h"
#include "fpipe.h"
#include "eval.h"

// default texture resource(s)

#define DEF_TEX_COUNT 1
TEX_RES gTexRes[DEF_TEX_COUNT] = { 
    { TEX_BMP, IDB_DEFTEX }
};

static void InitTexParams();

/******************************Public*Routine******************************\
* STATE constructor
*
* - global state init
* - translates variables set from the dialog boxes
*
\**************************************************************************/

//mf: since pass bXXX params why not do same with ulSurfStyle, fTesselFact,
// ulTexQual

STATE::STATE( BOOL bFlexMode, BOOL bMultiPipes )
{
    // various state values
    resetStatus = RESET_STARTUP_BIT;

    // Put initial hglrc in drawThreads[0]
    // This RC is also used for dlists and texture objects that are shared
    // by other RC's

    shareRC = wglGetCurrentContext();
    drawThreads[0].SetRCDC( shareRC, wglGetCurrentDC() );

    bTexture = FALSE;
    if( ulSurfStyle == SURFSTYLE_TEX ) {
        if( LoadTextureFiles( gTexFile, gnTextures, &gTexRes[0] ) )
            bTexture = TRUE;
    }
    else if( ulSurfStyle == SURFSTYLE_WIREFRAME ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }
   
    // Initialize GL state for the initial RC (sets texture state, so
    // (must come after LoadTextureFiles())

    GLInit();

    // set 'reference' radius value

    radius = 1.0f;

    // convert tesselation from fTesselFact(0.0-2.0) to tessLevel(0-MAX_TESS)

    int tessLevel = (int) (fTesselFact * (MAX_TESS+1) / 2.0001f);
    nSlices = (tessLevel+2) * 4;

    // Allocate basic NODE_ARRAY
    // NODE_ARRAY size is determined in Reshape (based on window size)
    nodes = new NODE_ARRAY;

    // Set drawing mode, and initialize accordingly.  For now, either all normal
    // or all flex pipes are drawn, but they could be combined later.
    // Can assume here that if there's any possibility that normal pipes
    // will be drawn, NORMAL_STATE will be initialized so that dlists are
    // built
    
    // Again, since have either NORMAL or FLEX, set maxPipesPerFrame,
    // maxDrawThreads
    if( bMultiPipes )
        maxDrawThreads = MAX_DRAW_THREADS;
    else
        maxDrawThreads = 1;
    nDrawThreads = 0; // no active threads yet
    nPipesDrawn = 0;
    // maxPipesPerFrame is set in Reset()

    if( bFlexMode ) {
        drawMode = DRAW_FLEX;
        pFState = new FLEX_STATE( this );
        pNState = NULL;
    } else {
        drawMode = DRAW_NORMAL;
        pNState = new NORMAL_STATE( this );
        pFState = NULL;
    }

    // initialize materials

    if( bTexture )
        ss_InitTexMaterials();
    else
        ss_InitTeaMaterials();

    // default draw scheme
    drawScheme = FRAME_SCHEME_RANDOM;
}

/******************************Public*Routine******************************\
* STATE destructor
*
\**************************************************************************/

STATE::~STATE( )
{
    if( pNState )
        delete pNState;
    if( pFState )
        delete pFState;
    if( nodes )
        delete nodes;
    if( bTexture ) {
        for( int i = 0; i < nTextures; i ++ ) {
            ss_DeleteTexture( &texture[i] );
        }
    }

    // Delete any RC's - should be done by ~THREAD, but since common lib
    // deletes shareRC, have to do it here

    DRAW_THREAD *pdt = &drawThreads[0];
    for( int i = 0; i < MAX_DRAW_THREADS; i ++, pdt++ ) {
        if( pdt->hglrc && (pdt->hglrc != shareRC) ) {
            wglDeleteContext( pdt->hglrc );
        }
    }
}

/******************************Public*Routine******************************\
* CalcTexRepFactors 
*
\**************************************************************************/

void
STATE::CalcTexRepFactors()
{
    ISIZE winSize;
    POINT2D texFact;

    ss_GetScreenSize( &winSize );

    // Figure out repetition factor of texture, based on bitmap size and
    // screen size.
    //
    // We arbitrarily decide to repeat textures that are smaller than
    // 1/8th of screen width or height.

    for( int i = 0; i < nTextures; i++ ) {
        texRep[i].x = texRep[i].y = 1;

        if( (texFact.x = winSize.width / texture[i].width / 8.0f) >= 1.0f)
            texRep[i].x = (int) (texFact.x+0.5f);

        if( (texFact.y = winSize.height / texture[i].height / 8.0f) >= 1.0f)
            texRep[i].y = (int) (texFact.y+0.5f);
    }
    
    // ! If display list based normal pipes, texture repetition is embedded
    // in the dlists and can't be changed. So use the smallest rep factors.
    // mf: Should change this so smaller textures are replicated close to
    // the largest texture, then same rep factor will work well for all
    
    if( pNState ) {
        //put smallest rep factors in texRep[0]; (mf:this is ok for now, as
        // flex pipes and normal pipes don't coexist)
    
        for( i = 1; i < nTextures; i++ ) {
            if( texRep[i].x < texRep[0].x )
                texRep[0].x = texRep[i].x;
            if( texRep[i].y < texRep[0].y )
                texRep[0].y = texRep[i].y;
        }
    } 
}

/******************************Public*Routine******************************\
* LoadTextureFiles
*
* - Load user texture files.  If texturing on but no user textures, or
*   problems loading them, load default texture resource
* mf: later, may want to have > 1 texture resource
*
\**************************************************************************/

BOOL
STATE::LoadTextureFiles( TEXFILE *pTexFile, int nTexFiles, TEX_RES *pTexRes )
{
    // Set pixel store state

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Try to load the bmp or rgb file

    // i counts successfully loaded textures
    for( int i = 0; nTexFiles; nTexFiles-- ) {
        if( ss_LoadTextureFile( &pTexFile[i], &texture[i] ) )  {
            // If texture object extension, set tex params here for each object
            if( ss_TextureObjectsEnabled() )
                InitTexParams();
            i++; // count another valid texture
        }
    }

    // set number of valid textures in state
    nTextures = i;

    if( nTextures == 0 ) {
        // No user textures, or none loaded successfully
        // Load default resource texture(s)
        nTextures = DEF_TEX_COUNT;
        for( i = 0; i < nTextures; i++, pTexRes++ ) {
            if( !ss_LoadTextureResource( pTexRes, &texture[i] ) ) {
                // shouldn't happen
                return FALSE;
            }
        }
    }

    CalcTexRepFactors();

    return TRUE;
}

/******************************Public*Routine******************************\
* GLInit
*
* - Sets up GL state
* - Called once for every context (rc)
*
\**************************************************************************/

void 
STATE::GLInit()
{
    static float ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    static float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float position[] = {90.0f, 90.0f, 150.0f, 0.0f};
    static float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float lmodel_ambientTex[] = {0.6f, 0.6f, 0.6f, 0.0f};
    static float back_mat_diffuse[] = {0.0f, 0.0f, 1.0f};

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glFrontFace(GL_CCW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glEnable( GL_AUTO_NORMAL ); // needed for GL_MAP2_VERTEX (tea)

    if( bTexture )
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambientTex);
    else
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

#if 1
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
#else
// debug
// back material for debugging
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
#endif
    
    // Set texture modes
    if( bTexture ) {
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        InitTexParams();
    }
}

/**************************************************************************\
* InitTexParams
*
* Set texture parameters, globally, or per object if texture object extension
*
\**************************************************************************/

static void
InitTexParams()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    switch( ulTexQuality ) {
       case TEXQUAL_HIGH:
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
       case TEXQUAL_DEFAULT:
       default:
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
    }
}


/**************************************************************************\
* Repaint
*
* This is called when a WM_PAINT msg has been sent to the window.   The paint
* will overwrite the frame buffer, screwing up the scene if pipes is in single
* buffer mode.  We set resetStatus accordingly to clear things up on next
* draw. 
*
\**************************************************************************/

void
STATE::Repaint( LPRECT pRect, void *data)
{
    resetStatus |= RESET_REPAINT_BIT;
}

/**************************************************************************\
* Reshape
*   - called on resize, expose
*   - always called on app startup
*   - set new window size for VIEW object, and set resetStatus for validation
*     at draw time
*
\**************************************************************************/

void
STATE::Reshape(int width, int height, void *data)
{
    if( view.SetWinSize( width, height ) )
        resetStatus |= RESET_RESIZE_BIT;
}

/**************************************************************************\
* ResetView
*
* Called on FrameReset resulting from change in viewing paramters (e.g. from
* a Resize event).
\**************************************************************************/

void
STATE::ResetView()
{
    IPOINT3D numNodes;

    // Have VIEW calculate the node array size based on view params
    view.CalcNodeArraySize( &numNodes );

    // Resize the node array
    nodes->Resize( &numNodes );

    // Set GL viewing parameters for each active RC

    DRAW_THREAD *pThread = drawThreads;

    for( int i = 0; i < MAX_DRAW_THREADS; i ++, pThread++ ) {
        if( pThread->HasRC() ) {
            pThread->MakeRCCurrent();
            view.SetGLView();
        }
    }
}

/**************************************************************************\
* FrameReset
*
* Start a new frame of pipes
*
* The resetStatus parameter indicates what triggered the Reset.
*
\**************************************************************************/

static int PickRandomTexture( int i, int nTextures );

void 
STATE::FrameReset()
{    
    int i;
    float xRot, zRot;
    PIPE *pNewPipe;

#ifdef DO_TIMING
    Timer( TIMER_STOP );
#endif

    SS_DBGINFO( "Pipes STATE::FrameReset:\n" );

    // Kill off any active pipes ! (so they can shut down ok)

    DRAW_THREAD *pThread = drawThreads;
    for( i = 0; i < nDrawThreads; i ++, pThread++ ) {
        pThread->KillPipe();
    }
    nDrawThreads = 0;
    
    // Clear the screen
    Clear();

    // Check for window resize status
    if( resetStatus & RESET_RESIZE_BIT ) {
        ResetView();
    }

    // Reset the node states to empty
    nodes->Reset();

    // Call any pipe-specific state resets, and get any recommended
    // pipesPerFrame counts

    if( pNState ) {
        pNState->Reset();
    }
    if( pFState ) {
        pFState->Reset();
        //mf: maybe should figure out min spherical view dist
        xRot = ss_fRand(-5.0f, 5.0f);
        zRot = ss_fRand(-5.0f, 5.0f);
    }
    maxPipesPerFrame = CalcMaxPipesPerFrame();

    // Set new number of drawing threads

    if( maxDrawThreads > 1 ) {
        // Set maximum # of pipes per frame
        maxPipesPerFrame = (int) (maxPipesPerFrame * 1.5);

        // Set # of draw threads
        nDrawThreads = SS_MIN( maxPipesPerFrame, ss_iRand2( 2, maxDrawThreads ) );
        // Set chase mode if applicable, every now and then
        BOOL bUseChase = pNState || (pFState && pFState->OKToUseChase());
        if( bUseChase && (!ss_iRand(5)) ) {
            drawScheme = FRAME_SCHEME_CHASE;
        }
    } else {
        nDrawThreads = 1;
    }
    nPipesDrawn = 0;

    // for now, either all NORMAL or all FLEX for each frame

    pThread = drawThreads;

    for( i = 0; i < nDrawThreads; i ++, pThread++ ) {

        // Create hglrc if necessary, and init it

        if( !pThread->HasRC() ) {
            HDC hdc = wglGetCurrentDC();
            pThread->SetRCDC( wglCreateContext( hdc ), hdc );
            // also need to init each RC
            pThread->MakeRCCurrent();
#if 0
//mf: should get this working
            wglCopyContext( drawThreads[0].GetRC(), pThread->GetRC(), 0xffff );
#endif
            // Do GL Init for this new RC
            GLInit();

            // Set viewing params
            view.SetGLView();
            
            // Give this rc access to any dlists
            wglShareLists( shareRC, pThread->GetRC() );
        }
        else
            pThread->MakeRCCurrent();
        
        // Set up the modeling view

        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, view.zTrans);

        // Rotate Scene
        glRotatef( view.yRot, 0.0f, 1.0f, 0.0f );

        // create approppriate pipe for this thread slot

        switch( drawMode ) {
            case DRAW_NORMAL:
                pNewPipe = (PIPE *) new NORMAL_PIPE(this);
                break;
            case DRAW_FLEX:
                // There are several kinds of FLEX pipes - have FLEX_STATE
                // decide which one to create
                pNewPipe = pFState->NewPipe( this );
                // rotate a bit around x and z as well
                // mf: ! If combining NORMAL and FLEX, same rotations must be 
                // applied to both
                glRotatef( xRot, 1.0f, 0.0f, 0.0f );
                glRotatef( zRot, 0.0f, 0.0f, 1.0f ); 
                break;
        }
        pThread->SetPipe( pNewPipe );

        if( drawScheme == FRAME_SCHEME_CHASE ) {
            if( i == 0 ) {
                // this will be the lead pipe
                pLeadPipe = pNewPipe;
                pNewPipe->SetChooseDirectionMethod( CHOOSE_DIR_RANDOM_WEIGHTED );
            } else {
                pNewPipe->SetChooseDirectionMethod( CHOOSE_DIR_CHASE );
            }
        }

        // If texturing, pick a random texture for this thread

        if( bTexture ) {
            int index = PickRandomTexture( i, nTextures );
            pThread->SetTexture( &texture[index] );

            // Flex pipes need to be informed of the texture, so they 
            // can dynamically calculate various texture params
            if( pFState )
                ((FLEX_PIPE *) pNewPipe)->SetTexParams( &texture[index], 
                                                        &texRep[index] );
        }

        // Launch the pipe (assumed: always more nodes than pipes starting, so
        // StartPipe cannot fail)

        // ! All pipe setup needs to be done before we call StartPipe, as this
        // is where the pipe starts drawing

        pThread->StartPipe();

        // Kind of klugey, but if in chase mode, I set chooseStartPos here,
        // since first startPos used in StartPipe() should be random
        if( (i == 0) && (drawScheme == FRAME_SCHEME_CHASE) )
            pNewPipe->SetChooseStartPosMethod( CHOOSE_STARTPOS_FURTHEST );

        nPipesDrawn++;
    }

    // Increment scene rotation for normal reset case
    if( resetStatus & RESET_NORMAL_BIT )
        view.IncrementSceneRotation();

    // clear reset status
    resetStatus = 0;

#ifdef DO_TIMING
    Timer( TIMER_START );
#endif
}


/**************************************************************************\
* CalcMaxPipesPerFrame
*
\**************************************************************************/

int
STATE::CalcMaxPipesPerFrame()
{
    int nCount=0, fCount=0;

    if( pFState )
        fCount = pFState->GetMaxPipesPerFrame();
    if( pNState )
        nCount = bTexture ? NORMAL_TEX_PIPE_COUNT : NORMAL_PIPE_COUNT;
    return SS_MAX( nCount, fCount );
}

/**************************************************************************\
* PickRandomTexture
*
* Pick a random texture index from a list.  Remove entry from list as it
* is picked.  Once all have been picked, or starting a new frame, reset.
*
* ! Routine not reentrant, should only be called by the main thread
* dispatcher (FrameReset)
\**************************************************************************/

static int
PickRandomTexture( int iThread, int nTextures )
{
    if( nTextures == 0 )
        return 0;

    static int pickSet[MAX_TEXTURES] = {0};
    static int nPicked = 0;
    int i, index;

    if( iThread == 0 )
        // new frame - force reset
        nPicked = nTextures;

    // reset condition
    if( ++nPicked > nTextures ) {
        for( i = 0; i < nTextures; i ++ ) pickSet[i] = 0;
        nPicked = 1; // cuz
    }

    // Pick a random texture index
    index = ss_iRand( nTextures );
    while( pickSet[index] ) {
        // this index has alread been taken, try the next one
        if( ++index >= nTextures )
            index = 0;
    }
    // Hopefully, the above loop will exit :).  This means that we have
    // found a texIndex that is available
    pickSet[index] = 1; // mark as taken
    return index;
}

/**************************************************************************\
* Clear
*
* Clear the screen.  Depending on resetStatus, use normal clear or
* fancy transitional clear.
\**************************************************************************/

void 
STATE::Clear()
{
    // clear the screen - any rc will do

    glClear(GL_DEPTH_BUFFER_BIT);

    if( resetStatus & RESET_RESIZE_BIT ) {
        // new window size - recalibrate the transitional clear

        // Calibration is set after a window resize, so window is already black
        ddClear.CalibrateClear( view.winSize.width, view.winSize.height, 2.0f );
    } else if( resetStatus & RESET_NORMAL_BIT )
        // do the normal transitional clear
        ddClear.Clear( view.winSize.width, view.winSize.height );
    else {
        // do a fast one-shot clear
        glClear( GL_COLOR_BUFFER_BIT );
    }
}


/**************************************************************************\
* DrawValidate
*
* Validation done before every Draw
*
* For now, this just involves checking resetStatus
*
\**************************************************************************/

void 
STATE::DrawValidate()
{    
    if( ! resetStatus )
        return;

    FrameReset();
}

/**************************************************************************\
* Draw
*
* - Top-level pipe drawing routine
* - Each pipe thread keeps drawing new pipes until we reach maximum number
*   of pipes per frame - then each thread gets killed as soon as it gets
*   stuck.  Once number of drawing threads reaches 0, we start a new
*   frame
*
\**************************************************************************/

void 
STATE::Draw(void *data)
{
    int nKilledThreads = 0;
    BOOL bChooseNewLead = FALSE;

    // Validate the draw state

    DrawValidate();

    // Check each pipe's status

    DRAW_THREAD *pThread = drawThreads;

    for( int i = 0; i < nDrawThreads; i++, pThread++  ) {
        if( pThread->pPipe->IsStuck() ) {
            if( ++nPipesDrawn > maxPipesPerFrame ) {
                // Reaching pipe saturation - kill this pipe thread

                if( (drawScheme == FRAME_SCHEME_CHASE) &&
                    (pThread->pPipe == pLeadPipe) ) 
                    bChooseNewLead = TRUE;

                pThread->KillPipe();
                nKilledThreads++;

            } else {
                // Start up another pipe
                if( ! pThread->StartPipe() )
                    // we won't be able to draw any more pipes this frame
                    // (probably out of nodes)
                    maxPipesPerFrame = nPipesDrawn;
            }
        }
    }

    // Whenever one or more pipes are killed, compact the thread list
    if( nKilledThreads ) {
        CompactThreadList();
        nDrawThreads -= nKilledThreads;
    }

    if( nDrawThreads == 0 ) {
        // This frame is finished - mark for reset on next Draw
        resetStatus |= RESET_NORMAL_BIT;
        return;
    }

    if( bChooseNewLead ) {
        // We're in 'chase mode' and need to pick a new lead pipe
        ChooseNewLeadPipe();
    }

    // Draw each pipe

    for( i = 0, pThread = drawThreads; i < nDrawThreads; i++, pThread++ ) {
        pThread->DrawPipe();
#ifdef DO_TIMING
        pipeCount++;
#endif
    }

    glFlush();
}


/**************************************************************************\
*
* CompactThreadList
*
* - Compact the thread list according to number of pipe threads killed
* - The pipes have been killed, but the RC's in each slot are still valid
*   and reusable.  So we swap up entries with valid pipes. This means that
*   the ordering of the RC's in the thread list will change during the life
*   of the program.  This should be OK.
*
\**************************************************************************/

#define SWAP_SLOT( a, b ) \
    DRAW_THREAD pTemp; \
    pTemp = *(a); \
    *(a) = *(b); \
    *(b) = pTemp;
    
void
STATE::CompactThreadList()
{
    if( nDrawThreads <= 1 )
        // If only one active thread, it must be in slot 0 from previous
        // compactions - so nothing to do
        return;

    int iEmpty = 0;
    DRAW_THREAD *pThread = drawThreads;

    for( int i = 0; i < nDrawThreads; i ++, pThread++ ) {
        if( pThread->pPipe ) {
            if( iEmpty < i ) {
                // swap active pipe thread and empty slot
                SWAP_SLOT( &(drawThreads[iEmpty]), pThread );
            }
            iEmpty++;
        }
    }
}

/**************************************************************************\
*
* ChooseNewLeadPipe
*
* Choose a new lead pipe for chase mode.
*
\**************************************************************************/

void
STATE::ChooseNewLeadPipe()
{
    // Pick one of the active pipes at random to become the new lead

    int iLead = ss_iRand( nDrawThreads );
    pLeadPipe = drawThreads[iLead].pPipe;
    pLeadPipe->SetChooseStartPosMethod( CHOOSE_STARTPOS_FURTHEST );
    pLeadPipe->SetChooseDirectionMethod( CHOOSE_DIR_RANDOM_WEIGHTED );
}

/******************************Public*Routine******************************\
* Finish
*
* - Called when GL window being closed
*
\**************************************************************************/
void 
STATE::Finish( void *data )
{
    delete (STATE *) data;
}

/**************************************************************************\
* DRAW_THREAD constructor
*
\**************************************************************************/

DRAW_THREAD::DRAW_THREAD()
{
    hdc = 0;
    hglrc = 0;
    pPipe = NULL;
    htex = (HTEXTURE) -1;
}

/**************************************************************************\
* DRAW_THREAD destructor
*
* Delete any GL contexts
*
* - can't Delete shareRC, as this is done by common lib, so had to move
*   this up to ~STATE
*
\**************************************************************************/

DRAW_THREAD::~DRAW_THREAD()
{
#if 0
    wglDeleteContext( hglrc );
#endif
}

/**************************************************************************\
* MakeRCCurrent
*
\**************************************************************************/

void 
DRAW_THREAD::MakeRCCurrent()
{
    if( hglrc != wglGetCurrentContext() )
        wglMakeCurrent( hdc, hglrc );
}

/**************************************************************************\
* SetRCDC
*
\**************************************************************************/

void 
DRAW_THREAD::SetRCDC( HGLRC rc, HDC Hdc )
{
    hglrc = rc;
    hdc = Hdc;
}

/**************************************************************************\
* SetPipe
*
\**************************************************************************/

void 
DRAW_THREAD::SetPipe( PIPE *pipe )
{
    pPipe = pipe;
}

/**************************************************************************\
* HasRC
*
\**************************************************************************/

BOOL
DRAW_THREAD::HasRC()
{
    return( hglrc != 0 );
}

/**************************************************************************\
* GetRC
*
\**************************************************************************/

HGLRC
DRAW_THREAD::GetRC()
{
    return hglrc;
}

/**************************************************************************\
* SetTexture
*
* - Set a texture for a thread
* - Cache the texture index for performance
\**************************************************************************/

void 
DRAW_THREAD::SetTexture( HTEXTURE hnewtex )
{
    if( hnewtex != htex )
    {
        htex = hnewtex;
        ss_SetTexture( htex );
    }
}


/**************************************************************************\
* DrawPipe
*
* - Draw pipe in thread slot, according to its type
*
\**************************************************************************/

void 
DRAW_THREAD::DrawPipe()
{
    MakeRCCurrent();

    switch( pPipe->type ) {
        case TYPE_NORMAL:
            ( (NORMAL_PIPE *) pPipe )->Draw();
            break;
        case TYPE_FLEX_REGULAR:
            ( (REGULAR_FLEX_PIPE *) pPipe )->Draw();
            break;
        case TYPE_FLEX_TURNING:
            ( (TURNING_FLEX_PIPE *) pPipe )->Draw();
            break;
    }
    glFlush();
}
/**************************************************************************\
* StartPipe
*
* Starts up pipe of the approppriate type.  If can't find an empty node
* for the pipe to start on, returns FALSE;
*
\**************************************************************************/

BOOL
DRAW_THREAD::StartPipe()
{
    MakeRCCurrent();

    // call pipe-type specific Start function

    switch( pPipe->type ) {
        case TYPE_NORMAL:
            ( (NORMAL_PIPE *) pPipe )->Start();
            break;
        case TYPE_FLEX_REGULAR:
            ( (REGULAR_FLEX_PIPE *) pPipe )->Start();
            break;
        case TYPE_FLEX_TURNING:
            ( (TURNING_FLEX_PIPE *) pPipe )->Start();
            break;
    }
    glFlush();

    // check status
    if( pPipe->NowhereToRun() )
        return FALSE;
    else
        return TRUE;
}

/**************************************************************************\
* KillPipe
*
\**************************************************************************/

void 
DRAW_THREAD::KillPipe()
{
    switch( pPipe->type ) {
        case TYPE_NORMAL:
            delete (NORMAL_PIPE *) pPipe;
            break;
        case TYPE_FLEX_REGULAR:
            delete (REGULAR_FLEX_PIPE *) pPipe;
            break;
        case TYPE_FLEX_TURNING:
            delete (TURNING_FLEX_PIPE *) pPipe;
            break;
    }
    pPipe = NULL;
}
