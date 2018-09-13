/******************************Module*Header*******************************\
* Module Name: sstext3d.c
*
* Core code for text3D screen saver
*
* Created: 12-24-94 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <windows.h>
#include <scrnsave.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glaux.h>

//#define SS_DEBUG 1

#include "sscommon.h"
#include "sstext3d.h"

#define FMAX_CHORDAL_DEVIATION   0.008f

#define FMIN_DEPTH           0.15f
#define FMAX_DEPTH           0.6f

#define FMIN_VIEW_ANGLE      90.0f
#define FMAX_VIEW_ANGLE     130.0f

#define FMIN_RANDOM_ANGLE    45.0f
#define FMAX_RANDOM_ANGLE    89.0f

#define FMIN_SEESAW_ANGLE    63.0f
#define FMAX_SEESAW_ANGLE    88.0f

#define FMIN_WOBBLE_ANGLE    30.0f
#define FMAX_WOBBLE_ANGLE    55.0f
#define FMIN_WOBBLE_ANGLE2   40.0f
#define FMAX_WOBBLE_ANGLE2   80.0f

#define MIN_ROT_STEP          1
#define MAX_ROT_STEP         20

#define FMAX_ZOOM             5.0f

// globals

static FLOAT gfMinCycleTime = 10.0f;
static POINTFLOAT gTrig[360];  // pre-calculated table of sines and cosines
static POINTFLOAT gSawTooth[360]; // sawtooth table
static POINTFLOAT gInvTrig[360];  // pseudo-inverse trig table
static POINT gTrigDif[360];   // table for converting trig->invtrig
static POINT gInvTrigDif[360];   // table for converting invtrig->trig

AttrContext gac;

// Default texture resource
TEX_RES gTexRes = { TEX_BMP, IDB_DEFTEX };

typedef struct _LIST *PLIST;
typedef struct _LIST {
    PLIST pnext;
    PLIST plistComplete;
    LPTSTR pszStr;
} LIST;

PLIST gplistComplete = NULL;
PLIST gplist = NULL;
static void DeleteNameList();

void text3d_Init( void *data  );
void text3d_Reset(void *data );
void text3d_Draw(void *data );
void text3d_Reshape(int width, int height, void *data );
void text3d_Finish( void *data );
static void CalcViewParams( AttrContext *pac );
static BOOL InitFont( AttrContext *pac );
static void InitLighting( AttrContext *pac );
static void InitTexture( AttrContext *pac );
static void InitMaterials( AttrContext *pac );
static void InitView( AttrContext *pac );
static FLOAT MapValue( FLOAT fInVal,
                       FLOAT fIn1, FLOAT fIn2,
                       FLOAT fOut1, FLOAT fOut2 );
static int  MapValueI( int inVal, int in1, int in2, int out1, int out2 );
static FLOAT CalcChordalDeviation( HDC hdc, AttrContext *pac );
static void (*BoundingBoxProc)( AttrContext *pac);
static void CalcBoundingBox( AttrContext *pac );
static void CalcBoundingBoxFromSphere( AttrContext *pac );
static void CalcBoundingBoxFromSpherePlus( AttrContext *pac, FLOAT zmax );
static void CalcBoundingBoxGeneric( AttrContext *pac );
static void CalcBoundingBoxFromExtents( AttrContext *pac, POINT3D *box );
static void CalcBoundingExtent( FLOAT rot, FLOAT x, FLOAT y,
                                POINTFLOAT *extent );
static void SortBoxes( POINT3D *box, FLOAT *boxvp, int numBox );
static void (*GetNextRotProc)( AttrContext *pac );
static void GetNextRotNone( AttrContext *pac );
static void GetNextRotRandom( AttrContext *pac );
static void GetNextRotWobble( AttrContext *pac );
static void InitTrigTable();
static void text3d_UpdateTime( AttrContext *pac, BOOL bCheckBounds );
static void text3d_UpdateString( AttrContext *pac, BOOL bCheckBounds );
static BOOL VerifyString( AttrContext *pac );
static BOOL CheckKeyStrings( LPTSTR testString, PSZ psz );
static void ConvertStringAsciiToUnicode( PSZ psz, PWSTR pwstr, int len );
static void InvertBitsA( char *s, int len );
static void ReadNameList();
static PSZ ReadStringFileA( char *file );
static void CreateRandomList();
static void ResetRotationLimits( AttrContext *pac, int *reset );
static int FrameCalibration( AttrContext *pac, struct _timeb *pBaseTime, int framesPerCycle,
                             int nCycle );
static void SetTransitionPoints( AttrContext *pac, int framesPerCycle,
                                int *trans1, int *trans2, FLOAT *zTrans );
static void
AdjustRotationStep( AttrContext *pac, int *reset, POINTFLOAT *oldTrig );

/******************************Public*Routine******************************\
* SetFloaterInfo
*
* Set the size and motion of the floating window
*
* ss_SetWindowAspectRatio may be called after this, to finely crop the
* window to the text being displayed.  But we can't call it here, since this
* function is called by common when creating the floating window, before the
* text string size has been determined.
\**************************************************************************/

static void
SetFloaterInfo( ISIZE *pParentSize, CHILD_INFO *pChild )
{
    float sizeFact;
    float sizeScale;
    int size;
    ISIZE *pChildSize = &pChild->size;
    MOTION_INFO *pMotion = &pChild->motionInfo;
    AttrContext *pac = &gac;

    sizeScale = (float)pac->uSize / 100.0f;  // range 0..1
    sizeFact = 0.25f + (0.5f * sizeScale);     // range 25-75%
    size = (int) (sizeFact * 
            ( ((float)(pParentSize->width + pParentSize->height)) / 2.0f ));
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->width );
    SS_CLAMP_TO_RANGE2( size, 0, pParentSize->height );

    pChildSize->width = pChildSize->height = size;
    pMotion->posInc.x = .01f * (float) size;
    if( pMotion->posInc.x < 1.0f )
        pMotion->posInc.x = 1.0f;
    pMotion->posInc.y = pMotion->posInc.x;
    pMotion->posIncVary.x = .4f * pMotion->posInc.x;
    pMotion->posIncVary.y = pMotion->posIncVary.x;
}

/******************************Public*Routine******************************\
* Init
*
* Initialize - called on first entry into ss.
* Called BEFORE gl is initialized!
* Just do basic stuff here, like set up callbacks, verify dialog stuff, etc.
*
* Fills global SSContext structure with required data, and returns ptr
* to it.
*
\**************************************************************************/

SSContext *
ss_Init( void )
{
    // validate some initial dialog settings
    getIniSettings();  // also called on dialog init

    // must verify textures here, before GL floater windows are created
    if( gac.surfStyle == SURFSTYLE_TEX ) {
        ss_DisableTextureErrorMsgs();
        ss_VerifyTextureFile( &gac.texFile );
    }

    // set Init callback
    ss_InitFunc( text3d_Init );

    // set data ptr to be sent with callbacks
    ss_DataPtr( &gac );

    // set configuration info to return

    gac.ssc.bFloater = TRUE;
    gac.ssc.floaterInfo.bMotion = TRUE;
    gac.ssc.floaterInfo.ChildSizeFunc = SetFloaterInfo;

    gac.ssc.bDoubleBuf = TRUE;
    gac.ssc.depthType = SS_DEPTH16;

    return &gac.ssc;
}

/******************************Public*Routine******************************\
* text3d_Init
*
* Initializes OpenGL state for text3d screen saver
*
\**************************************************************************/
void
text3d_Init( void *data )
{
    AttrContext *pac = (AttrContext *) data;

    // Set any callbacks that require GL
    ss_UpdateFunc( text3d_Draw );
    ss_ReshapeFunc( text3d_Reshape );
    ss_FinishFunc( text3d_Finish );

#ifdef SS_DEBUG
    glClearColor( 0.2f, 0.2f, 0.2f, 0.0f );
#else
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#endif

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    // this sequence must be maintained

    InitLighting( pac );

    InitFont( pac );

    InitView( pac );

    InitTexture( pac );

    InitMaterials( pac );
}

/**************************************************************************\
* InitLighting
*
* Initialize lighting, and back face culling.
*
\**************************************************************************/
static void
InitLighting( AttrContext *pac )
{
    float ambient1[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float ambient2[] = {0.1f, 0.1f, 0.1f, 1.0f};
    float diffuse1[] = {0.7f, 0.7f, 0.7f, 1.0f};
    float diffuse2[] = {0.7f, 0.7f, 0.7f, 1.0f};
    float position1[] = {0.0f, 50.0f, 150.0f, 0.0f};
    float position2[] = {25.0f, 150.0f, 50.0f, 0.0f};
    float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse1);
    glLightfv(GL_LIGHT0, GL_POSITION, position1);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse2);
    glLightfv(GL_LIGHT1, GL_POSITION, position2);
    glEnable(GL_LIGHT1);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glCullFace( GL_BACK );
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

/**************************************************************************\
* TestFont
*
* Test that GetOutlineTextMetrics works.  If not, wglUseFontOutlines will fail.
*
* If the font tests bad, delete it and select in the previous one.
*
\**************************************************************************/

static BOOL
TestFont( HFONT hfont )
{
    OUTLINETEXTMETRIC otm;
    HFONT hfontOld;
    HDC hdc = wglGetCurrentDC();

    hfontOld = SelectObject(hdc, hfont);

    if( GetOutlineTextMetrics( hdc, sizeof(otm), &otm) <= 0 ) {
        SS_DBGPRINT( "sstext3d Init: GetOutlineTextMetrics failure\n" );
        SelectObject(hdc, hfontOld);
        DeleteObject( hfont );
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************\
* CreateFont
*
* Create a true type font and test it
*
\**************************************************************************/

static HFONT
text3d_CreateFont( LOGFONT *plf )
{
    HFONT hfont;

    // Create font from LOGFONT data
    hfont = CreateFontIndirect(plf);

    if( hfont ) {
        // Test the font
        if( ! TestFont( hfont ) )
            hfont = (HFONT) 0;
    }
    return hfont;
}

/**************************************************************************\
* InitFont
*
*
\**************************************************************************/
static BOOL
InitFont( AttrContext *pac )
{
    LOGFONT lf;
    HFONT hfont;
    int type;
    float fChordalDeviation;
    HDC hdc = wglGetCurrentDC();

    // Set up the LOGFONT structure

    memset(&lf, 0, sizeof(LOGFONT));
    lstrcpy( lf.lfFaceName, pac->szFontName );
    lf.lfWeight = (pac->bBold) ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = (pac->bItalic) ? (BYTE) 1 : 0;

    lf.lfHeight = 0; // shouldn't matter
    lf.lfCharSet = pac->charSet;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    // Create the font

    if( ! (hfont = text3d_CreateFont( &lf )) ) {
        // Couldn't create a true type font with supplied data
        SS_DBGPRINT( "initial text3d_CreateFont failed: \n" );
    }

    if( !hfont && ss_fOnWin95() ) {
    // !!! Bug on win95: the font mapper didn't give us anything useful
    // For some reason GetOutlineTextMetrics fails for some fonts (Symbol)
    // when using lfHeight = 0 (default height value).
        lf.lfHeight = -10;
        if( ! (hfont = text3d_CreateFont( &lf )) ) {
            SS_DBGPRINT( "text3d_CreateFont with lfHeight != 0 failed: \n" );
        }
    }

    if( hfont == NULL ) {
        /* The requested font cannot be loaded.  Try to get the system to
         * load any TrueType font
         */
        hfont = CreateFont( 100, 100, 0, 0, lf.lfWeight, lf.lfItalic,
                             0, 0, 0, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, NULL );
        // If hfont is still null, nothing will be displayed.
        if( !hfont || !TestFont(hfont) ) {
            SS_DBGPRINT( "text3d_InitFont failure\n" );
            return FALSE;
        }
    }

    // We have a valid font

    SelectObject(hdc, hfont);

    // Set extrusion, chordal deviation, and font type

#ifdef _PPC_
    // !!! Work around for PPC compiler bug

    // calculate chordalDeviation from input attribute fTesselFact
    fChordalDeviation = CalcChordalDeviation( hdc, pac );

    pac->fDepth = ss_fRand( FMIN_DEPTH, FMAX_DEPTH );
#else
    pac->fDepth = ss_fRand( FMIN_DEPTH, FMAX_DEPTH );

    // calculate chordalDeviation from input attribute fTesselFact
    fChordalDeviation = CalcChordalDeviation( hdc, pac );
#endif

    type = pac->surfStyle == SURFSTYLE_WIREFRAME ? WGL_FONT_LINES :
                                                   WGL_FONT_POLYGONS;

    // Create a wgl font context

    if( !(pac->pWglFontC = 
            CreateWglFontContext( hdc, type, pac->fDepth, fChordalDeviation )) )
        return FALSE;

    // intialize the text that will be displayed

    if( pac->demoType == DEMO_CLOCK ) {
        text3d_UpdateTime( pac, FALSE );  // sets pac->textXXX params as well
    } else if( pac->demoType == DEMO_STRING ) {
        if( !VerifyString( pac ) ) {
            ConvertStringToList( pac->szText, pac->usText, pac->pWglFontC );
            pac->textLen = GetStringExtent( pac->szText, &pac->pfTextExtent,
                                         &pac->pfTextOrigin,
                                         pac->pWglFontC );
        }
    }
    return SUCCESS;
}


/**************************************************************************\
* InitView
*
*
\**************************************************************************/
static void
InitView( AttrContext *pac )
{
    int numRots=0, axis;
    FLOAT *p3dRotMax = (FLOAT *) &pac->p3dRotMax;
    FLOAT *p3dRotMin = (FLOAT *) &pac->p3dRotMin;
    int *ip3dRotStep = (int *) &pac->ip3dRotStep;
    POINT3D p3d_zero = {0.0f, 0.0f, 0.0f};
    int stepRange = 2; // default step range
    int reset[NUM_AXIS] = {1, 1, 1};

    // text is either xmajor or ymajor
    pac->bXMajor = pac->pfTextExtent.x >= pac->pfTextExtent.y ? TRUE : FALSE;


    /* At this point, the initial string extents will have been
     * calculated, and we can use this to determine rotational
     * characteristics
     */

    // default proc to get next rotation
    GetNextRotProc = GetNextRotRandom;

    /* convert the slider speed values to rotation steps, with
     * a steeper slope at the beginning of the scale
     */
    pac->iRotStep = MapValueI( pac->iSpeed,
                               MIN_SLIDER, MAX_SLIDER,  // slider range
                               MIN_ROT_STEP, MAX_ROT_STEP ); // step range

    // initialize rotation min/max to 0
    *( (POINT3D*)p3dRotMin ) = p3d_zero;
    *( (POINT3D*)p3dRotMax ) = p3d_zero;
    pac->p3dRot = p3d_zero;

    /* Set the MAXIMUM rotation limits.  This is required initially, in
     * order to set the bounding box
     */
    switch( pac->rotStyle ) {
        case ROTSTYLE_NONE:
            GetNextRotProc = GetNextRotNone;
            break;

        case ROTSTYLE_SEESAW:
            // rotate minor axis
            if( pac->demoType == DEMO_VSTRING )
                // always rotate around y-axis
                axis = Y_AXIS;
            else
                axis = pac->bXMajor ? Y_AXIS : X_AXIS;
            p3dRotMin[axis] = FMIN_SEESAW_ANGLE;
            p3dRotMax[axis] = FMAX_SEESAW_ANGLE;
            break;

        case ROTSTYLE_WOBBLE:
            GetNextRotProc = GetNextRotWobble;
            if( pac->demoType == DEMO_VSTRING ) {
                axis = Y_AXIS;
            }
            else {
                stepRange = 1;
                axis = pac->bXMajor ? Y_AXIS : X_AXIS;
            }
            p3dRotMin[Z_AXIS] = FMAX_WOBBLE_ANGLE;
            p3dRotMax[Z_AXIS] = FMAX_WOBBLE_ANGLE;
            p3dRotMin[axis] = FMIN_WOBBLE_ANGLE2;
            p3dRotMax[axis] = FMAX_WOBBLE_ANGLE2;
            break;

        case ROTSTYLE_RANDOM:
            // adjust stepRange based on speed
            stepRange = MapValueI( pac->iSpeed,
                               MIN_SLIDER, (MAX_SLIDER-MIN_SLIDER)/2,
                               2, 6 ); // step range
            for( axis = X_AXIS; axis < NUM_AXIS; axis++ ) {
                p3dRotMin[axis] = FMIN_RANDOM_ANGLE;
                p3dRotMax[axis] = FMAX_RANDOM_ANGLE;
            }
            break;
    }

    // set min and max steps
    pac->iRotMinStep = pac->iRotStep >= (MIN_ROT_STEP + stepRange) ?
              pac->iRotStep - stepRange : MIN_ROT_STEP;
    pac->iRotMaxStep = pac->iRotStep + stepRange; // don't limit upper end

    for( axis = X_AXIS; axis < NUM_AXIS; axis++ ) {
        ip3dRotStep[axis] = p3dRotMax[axis] != 0.0f ?
                ss_iRand2( pac->iRotMinStep, pac->iRotMaxStep ) : 0;
    }

    // initialize the step iteration
    pac->ip3dRoti.x = pac->ip3dRoti.y = pac->ip3dRoti.z = 0;

    // initialize the trig table, for fast rotation calculations
    InitTrigTable();

    // set the current rotation limits
    pac->p3dRotLimit = *( (POINT3D *)p3dRotMax );
    ResetRotationLimits( pac, reset );

    // set view angle
    pac->fFovy = ss_fRand( FMIN_VIEW_ANGLE, FMAX_VIEW_ANGLE );

    for( axis = X_AXIS; axis < NUM_AXIS; axis++ ) {
        if( p3dRotMax[axis] != 0.0f )
            numRots++;
    }

    // set BoundingBoxProc dependent on which axis are being rotated
    if( numRots <= 1 )
        BoundingBoxProc = CalcBoundingBox;
    else
        BoundingBoxProc = CalcBoundingBoxGeneric;

    (*BoundingBoxProc)( pac );
    if( pac->p3dBoundingBox.y == 0.0f )
        pac->p3dBoundingBox.y = 1.0f;
}

/**************************************************************************\
* InitMaterials
*
*
\**************************************************************************/
static void
InitMaterials( AttrContext *pac )
{
    if( pac->bTexture ) {
        ss_InitTexMaterials();
        pac->bMaterialCycle = FALSE;
        pac->pMat = ss_RandomTexMaterial( TRUE );
    } else {
        ss_InitTeaMaterials();
        pac->bMaterialCycle = TRUE;
        pac->pMat = ss_RandomTeaMaterial( TRUE );
    }
}

/**************************************************************************\
* InitTexture
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Changed texture quality from default to high.
*
\**************************************************************************/
static void
InitTexture( AttrContext *pac )
{
    if( pac->surfStyle != SURFSTYLE_TEX )
        return;

    // No choice for texture quality in dialog - set to HIGH
    pac->texQual = TEXQUAL_HIGH;

    // Try to load the texture file or default texture resource

    if( ss_LoadTextureFile( &pac->texFile, &pac->texture ) ||
        ss_LoadTextureResource( &gTexRes, &pac->texture ) ) 
    {
        pac->bTexture = 1;

        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        ss_SetTexture( &pac->texture );

        // set auto texture coord generation
        ss_InitAutoTexture( NULL );
    }
    else {  // couldn't open .bmp file
        pac->bTexture = 0;
    }

}

/******************************Public*Routine******************************\
* text3d_Finish
*
* Handles any cleanup on program termination
*
\**************************************************************************/
void
text3d_Finish( void *data )
{
    AttrContext *pac = (AttrContext *) data;

    if( pac )
        DeleteWglFontContext( pac->pWglFontC );

    // delete any name list
    DeleteNameList();
}

/**************************************************************************\
* text3d_Reshape
*
*       - called on resize, expose
*       - always called on app startup
*
\**************************************************************************/

void
text3d_Reshape(int width, int height, void *data )
{
    AttrContext *pac = (AttrContext *) data;

//mf
#if 0
    glViewport( 0, 0, width, height );
#endif

    // calculate new aspect ratio
    pac->fAspect = height == 0 ? 1.0f : (FLOAT) width / (FLOAT) height;

    CalcViewParams( pac );
    ss_SetWindowAspectRatio( pac->p3dBoundingBox.x / pac->p3dBoundingBox.y );
}


/**************************************************************************\
* CalcViewParams
*
*   Calculate viewing parameters, based on window size, bounding box, etc.
*
\**************************************************************************/

static void
CalcViewParams( AttrContext *pac )
{
    GLdouble zNear, zFar;
    FLOAT aspectBound, viewDist;
    FLOAT vHeight;
    FLOAT fovy;

    // calculate viewing distance so that front of bounding box within view

    aspectBound = pac->p3dBoundingBox.x / pac->p3dBoundingBox.y;

    // this is distance to FRONT of bounding box:
    viewDist = pac->p3dBoundingBox.y /
               ( (FLOAT) tan( deg_to_rad(pac->fFovy/2.0f) ) );

    // NOTE: these are half-widths and heights
    if( aspectBound <= pac->fAspect ) {
        // we are bound by the window's height
        fovy = pac->fFovy;
    } else {
        // we are bound by window's width
        // adjust fovy, so fovx remains the same
        vHeight = pac->p3dBoundingBox.x / pac->fAspect;
        fovy = rad_to_deg( 2.0f * (FLOAT) atan( vHeight / viewDist ) );
    }

    /* Could just use rotation sphere dimensions here, but for now
     * set clipping planes 10% beyond bounding box.
     */

    zNear = 0.9f * viewDist;
    zFar = 1.1f * (viewDist + 2.0f*pac->p3dBoundingBox.z);

    if( pac->demoType == DEMO_VSTRING )
        zFar *= FMAX_ZOOM;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective( fovy, pac->fAspect, zNear, zFar );

    // set viewing distance so that front of bounding box within view
    viewDist *= 1.01f; // pull back 1% further to be sure not off by a pixel..
    pac->fZtrans = -(viewDist + pac->p3dBoundingBox.z);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef( 0.0f, 0.0f, pac->fZtrans );

}


// number of calibration cycles
#define MAX_CALIBRATE 2

/**************************************************************************\
* text3d_Draw
*
* Draw a frame.
*
\**************************************************************************/
void
text3d_Draw( void *data )
{
    AttrContext *pac = (AttrContext *) data;
    POINT3D *rot = &pac->p3dRot;
    static BOOL bCalibrated = FALSE;
    static int nCycle = 0; // cycle count
    static int frameCount = 0, maxCount;
    static int matTransCount, matTransCount2 = 0;
    static MATERIAL transMat, transMatInc;
    static FLOAT zTrans, zTransInc;
    static MATERIAL *pNewMat;
    static struct _timeb baseTime;
    static BOOL bInit = FALSE;
    static int reset[NUM_AXIS] = {1,1,1};

    if( !bInit ) {

        // Do first time init stuff

        // Start the calibration timer
        _ftime( &baseTime );

        // set default transition points, until calibration done
        maxCount = 60;
        SetTransitionPoints( pac, maxCount, &matTransCount,
                             &matTransCount2, &zTrans );
        bInit = TRUE;
    }

    // take action based on frameCount

    if( frameCount >= matTransCount ) {

        // we are in the transition zone

        if( frameCount == matTransCount ) {

            // first transition point...

            // select new material
            if( pac->bTexture )
                pNewMat = ss_RandomTexMaterial( FALSE );
            else
                pNewMat = ss_RandomTeaMaterial( FALSE );

            // set material transition, zTrans transition
            if( pac->demoType == DEMO_VSTRING ) {
                // transition current material to black
                ss_CreateMaterialGradient( &transMatInc,
                                          pac->pMat,
                                          &ss_BlackMat,
                                          matTransCount2 - matTransCount );
                zTransInc = ((FMAX_ZOOM-1) * pac->fZtrans) /
                                        (matTransCount2 - matTransCount);
            } else {
                ss_CreateMaterialGradient( &transMatInc,
                                           pac->pMat,
                                           pNewMat,
                                           maxCount - matTransCount );
            }
            // initialize transition values to current settings
            zTrans = pac->fZtrans;
            transMat = *(pac->pMat);
            // begin transition on NEXT frame.

        } else {

            // past first transition...

            if( matTransCount2 && (frameCount == (matTransCount2+1)) ) {

                // optional second transition point...(only for vstrings)

                // transition from black to new material
                ss_CreateMaterialGradient( &transMatInc,
                                           &ss_BlackMat,
                                           pNewMat,
                                           maxCount - matTransCount2 );
                // init transition material to black
                transMat = ss_BlackMat;

                /* At this point, screen is black, so we can change strings
                 * and resize the floater without any problems.
                 */

                if( pac->demoType == DEMO_VSTRING )
                    text3d_UpdateString( pac, TRUE ); // can cause resize
                // set zTrans to furthest distance
                zTrans = (FMAX_ZOOM * pac->fZtrans);
                zTransInc = (pac->fZtrans - zTrans) /
                                            (maxCount - matTransCount2);
                // change this while string invisible
                ResetRotationLimits( pac, reset );
            }
            // set the transition material (updates transMat each time)

            ss_TransitionMaterial( &transMat, &transMatInc );
            zTrans += zTransInc;

            if( frameCount >= maxCount ) {

                // End of cycle
                nCycle++;  // 1-based

                // Calibrate on MAX_CALIBRATE cycles
                if( !bCalibrated && (nCycle >= MAX_CALIBRATE) ) {
                    maxCount = FrameCalibration( pac, &baseTime, maxCount, nCycle );

                    SetTransitionPoints( pac, maxCount, &matTransCount,
                                         &matTransCount2, &zTrans );
                    bCalibrated = TRUE;
                }

                // set, reset stuff
                pac->pMat = pNewMat;
                ss_SetMaterial( pNewMat );
                zTrans = pac->fZtrans;
                frameCount = 0;
            }
        }
    }

    if( pac->demoType == DEMO_CLOCK ) {
        // have to update the draw string with current time
        text3d_UpdateTime( pac, TRUE );
    }

    // ok, the string's setup - draw it

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if( pac->demoType == DEMO_VSTRING )
        // use zooming zTrans
        glTranslatef( 0.0f, 0.0f, zTrans );
    else
        // use fixed zTrans
        glTranslatef( 0.0f, 0.0f, pac->fZtrans );

    /*
     * GetNextRotProc provides sinusoidal rotations
     */
    (*GetNextRotProc)( pac );  // sets pac->p3dRot, or rot

    if( pac->p3dRotMax.z != 0.0f ) {
        glRotatef( rot->z, 0.0f, 0.0f, 1.0f );
    }
    if( pac->p3dRotMax.y != 0.0f ) {
        glRotatef( rot->y, 0.0f, 1.0f, 0.0f );
    }
    if( pac->p3dRotMax.x != 0.0f ) {
        glRotatef( rot->x, 1.0f, 0.0f, 0.0f );
    }

    glTranslatef( -pac->pfTextOrigin.x - pac->pfTextExtent.x/2.0f,
                  -pac->pfTextOrigin.y + pac->pfTextExtent.y/2.0f,
                  pac->fDepth / 2.0f );

    DrawString( pac->usText, pac->textLen, pac->pWglFontC );

    glFlush();
    frameCount++;
}

/**************************************************************************\
* SetTransitionPoints
*
* Calculate draw transition points, as frame count values.
*
* If doing variable string (VSTRING), first transition point is where we
* start fading to black, and 2nd is from black to next material.  Also
* transition the z translation distance (zTrans).
* Note that trans2 indicates the frame number where the image should be
* black.  The actual transitioning may occur at trans2+1 (see text3d_draw
* above).
*
* For all other cases, set one transition point for fade to next material.
*
\**************************************************************************/
static void
SetTransitionPoints( AttrContext *pac, int framesPerCycle, int *trans1,
                     int *trans2, FLOAT *zTrans )
{
    *trans1 = (int) (0.5f * (FLOAT) framesPerCycle + 0.5f);

    if( pac->demoType == DEMO_VSTRING ) {
        *trans2 = *trans1 +
                  (int) (0.5f * (FLOAT) (framesPerCycle - *trans1) + 0.5f);
        *zTrans = pac->fZtrans;
    }
}

/**************************************************************************\
* text3d_UpdateTime
*
* Put new time string into the attribute context
*
\**************************************************************************/
static void
text3d_UpdateTime( AttrContext *pac, BOOL bCheckBounds )
{
    int oldLen;
    POINTFLOAT textExtent, textOrigin;
    POINTFLOAT textLowerRight, currentLowerRight;
    LPTSTR pszLastTime = pac->szText;
    static TCHAR szNewTime[TEXT_BUF_SIZE] = {0};

    GetLocalTime( &(pac->stTime) );
    GetTimeFormat( GetUserDefaultLCID(), // locale id
                    0,                      // flags
                    &(pac->stTime),         // time struct
                    NULL,                   // format string
                    szNewTime,               // buffer
                    TEXT_BUF_SIZE );        // buffer size

    // Compare new time string with last one

    if( !lstrcmp( pszLastTime, szNewTime ) )
        // time string has not changed, return
        return;

    // translate the new time string into display lists in pac->usText

    ConvertStringToList( szNewTime, pac->usText, pac->pWglFontC );
    lstrcpy( pac->szText, szNewTime );

    // Check extents of new string

    // save current values
    oldLen = pac->textLen;
    textExtent = pac->pfTextExtent;
    textOrigin = pac->pfTextOrigin;

    pac->textLen = GetStringExtent( pac->szText,
                                     &textExtent,
                                     &textOrigin,
                                     pac->pWglFontC );

    if( !bCheckBounds ) {
        // just set new extents and return
        pac->pfTextExtent = textExtent;
        pac->pfTextOrigin = textOrigin;
        return;
    }

    /* only update bounding box if new extents are larger, or the number
     * of chars changes
     */
    bCheckBounds = FALSE;

    if( pac->textLen != oldLen ) {

        // recalculate everything

        bCheckBounds = TRUE;
        pac->pfTextExtent = textExtent;
        pac->pfTextOrigin = textOrigin;
    }
    else {

        // accumulate maximum bounding box in pac

        // calc current lower right limits
        textLowerRight.x = textOrigin.x + textExtent.x;
        textLowerRight.y = textOrigin.y - textExtent.y;
        currentLowerRight.x = pac->pfTextOrigin.x + pac->pfTextExtent.x;
        currentLowerRight.y = pac->pfTextOrigin.y - pac->pfTextExtent.y;

        // if new text extents extend beyond current, update

        if( textOrigin.x < pac->pfTextOrigin.x ) {
            pac->pfTextOrigin.x = textOrigin.x;
            bCheckBounds = TRUE;
        }
        if( textOrigin.y > pac->pfTextOrigin.y ) {
            pac->pfTextOrigin.y = textOrigin.y;
            bCheckBounds = TRUE;
        }
        if( textLowerRight.x > currentLowerRight.x ) {
            pac->pfTextExtent.x = textLowerRight.x - pac->pfTextOrigin.x;
            bCheckBounds = TRUE;
        }
        if( textLowerRight.y < currentLowerRight.y ) {
            pac->pfTextExtent.y = pac->pfTextOrigin.y - textLowerRight.y;
            bCheckBounds = TRUE;
        }
    }
    if( bCheckBounds ) {
        // string size has changed - recalc box and view params
        (*BoundingBoxProc)( pac );
        CalcViewParams( pac );
    }
}

/**************************************************************************\
* text3d_UpdateString
*
* Select new string to display.
* If bCheckBounds, calculate new bounds as well.
*
\**************************************************************************/
static void
text3d_UpdateString( AttrContext *pac, BOOL bCheckBounds )
{
    static int index = 0;

    // get next string to display

    if( gplist == NULL )
        CreateRandomList();

    lstrcpy( pac->szText, gplist->pszStr );
    ConvertStringToList( pac->szText, pac->usText, pac->pWglFontC );
    gplist = gplist->pnext;

    // get new extents
    pac->textLen = GetStringExtent( pac->szText,
                                     &pac->pfTextExtent,
                                     &pac->pfTextOrigin,
                                     pac->pWglFontC );

    if( !bCheckBounds )
        return;

    // calculate bounding box
    (*BoundingBoxProc)( pac );
    if( pac->p3dBoundingBox.y == 0.0f )
        // avoid /0
        pac->p3dBoundingBox.y = 1.0f;

    // Make window's aspect ratio dependent on bounding box

    /* mf: could clear buffer here, so don't get incorrect results on
     *  synchronous resize, but not necessary since we're fading
     */
    ss_SetWindowAspectRatio( pac->p3dBoundingBox.x / pac->p3dBoundingBox.y );
    CalcViewParams( pac );

    // move window to new random position
    ss_RandomWindowPos();
}

/**************************************************************************\
* GetNextRotWobble
*
*   Calculate next rotation.
*   - 'step' controls amount of rotation
*   - rotation values are scaled from -1 to 1 (trig values), and  inscribe
*     circle with r=1 in the zy plane for the ends of the string
*   - steps for both minor and major rotation axis remain in sync
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Call ResetRotationLimits() when axis rotation is 0
*
\**************************************************************************/
static void
GetNextRotWobble( AttrContext *pac )
{
    int *step = (int *) &pac->ip3dRoti;  // use step->x
    int *rotStep = (int *) &pac->ip3dRotStep.z;
    FLOAT *rotMax = (FLOAT *) &pac->p3dRotMax;
    POINTFLOAT *pTrig = pac->pTrig;
    static int resetPoint[NUM_AXIS] = {90,90,0}; // 0 amplitude points
    int reset[NUM_AXIS] = {0}; // which axis to be reset
    int axis;

    pac->p3dRot.z = pac->p3dRotLimit.z * pTrig[ *step ].y;  // sin
    pac->p3dRot.y = pac->p3dRotLimit.y * pTrig[ *step ].x;  // cos
    pac->p3dRot.x = pac->p3dRotLimit.x * pTrig[ *step ].x;  // cos

    // check for 0 amplitude point for non-vstrings
    for( axis = X_AXIS; axis < NUM_AXIS; axis++ ) {
        if( rotMax[axis] != 0.0f ) {
            if( (pac->demoType != DEMO_VSTRING) &&
                (*step == resetPoint[axis]) )
            {
                reset[axis] = 1;
                ResetRotationLimits( pac, reset );
                reset[axis] = 0;
            }
        }
    }

    // increment step
    if( (*step += *rotStep) >= 360 ) {
        // make the step variable
        *rotStep = ss_iRand2( pac->iRotMinStep, pac->iRotMaxStep );
        // start step at variable index
        *step = ss_iRand( *rotStep );
    }
}

/**************************************************************************\
* GetNextRotRandom
*
* Same as above, but steps for each axis are not kept in sync
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Call ResetRotationLimits() when axis rotation is 0
*
\**************************************************************************/
static void
GetNextRotRandom( AttrContext *pac )
{
    int *step = (int *) &pac->ip3dRoti;
    int *rotStep = (int *) &pac->ip3dRotStep;
    FLOAT *rotMax = (FLOAT *) &pac->p3dRotMax;
    POINTFLOAT *pTrig = pac->pTrig;
    static int resetPoint[NUM_AXIS] = {90,90,0}; // 0 amplitude points
    int reset[NUM_AXIS] = {0}; // which axis to be reset
    int axis;

    // set new rotation
    pac->p3dRot.z = pac->p3dRotLimit.z * pTrig[ step[Z_AXIS] ].y;  // sin
    pac->p3dRot.y = pac->p3dRotLimit.y * pTrig[ step[Y_AXIS] ].x;  // cos
    pac->p3dRot.x = pac->p3dRotLimit.x * pTrig[ step[X_AXIS] ].x;  // cos

    // for each rotation axis...

    for( axis = X_AXIS; axis < NUM_AXIS; axis++ ) {
        if( rotMax[axis] != 0.0f ) {
            // check for 0 amplitude point for non-vstrings
            if( (pac->demoType != DEMO_VSTRING) &&
                (step[axis] == resetPoint[axis]) ) {
                reset[axis] = 1;
                ResetRotationLimits( pac, reset );
                reset[axis] = 0;
            }

            // increment rotation step and check for end of cycle
            if( (step[axis] += rotStep[axis]) >= 360 ) {
                // make the step variable
                rotStep[axis] = ss_iRand2( pac->iRotMinStep, pac->iRotMaxStep );
                // start step at variable index
                step[axis] = ss_iRand( rotStep[axis] );
            }
        }
    }
}

/**************************************************************************\
* GetNextRotNone
*
* Null rot proc
*
\**************************************************************************/
static void
GetNextRotNone( AttrContext *pac )
{
}

/**************************************************************************\
* ResetRotationLimits
*
* Reset the maximum axis rotations.  So there won't be too much of a 'jump'
* when altering the rotation, this routine should only be called when the
* rotation of the specified axis is at zero amplitude.
*
* Also, change the rotation table if applicable.  This affects how the
* rotation 'steps' around the axis.
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Make rotation limits randomnly more extreme
*    - If trig table switched, call AdjustRotationStep(), to reset the steps
*      of non-zero axis rotations so that text string doesn't 'jump'
*
\**************************************************************************/

static void
ResetRotationLimits( AttrContext *pac, int *reset )
{
    FLOAT *p3dRot    =  (FLOAT *) &pac->p3dRot;       // current rotation
    FLOAT *p3dRotL   =  (FLOAT *) &pac->p3dRotLimit;  // new rot limit
    FLOAT *p3dRotMin =  (FLOAT *) &pac->p3dRotMin;    // max rotation
    FLOAT *p3dRotMax =  (FLOAT *) &pac->p3dRotMax;    // max rotation
    POINT3D p3dOldRotL = pac->p3dRotLimit; // save last rot limit
    POINTFLOAT *oldTrig;
    int i;

    // change rotation limits

    for( i = 0; i < NUM_AXIS; i++ ) {
        if( p3dRotMax[i] && reset[i] ) {
            p3dRotL[i] = ss_fRand( p3dRotMin[i], p3dRotMax[i] );
            // grossly modify amplitute sometimes for random
            if( pac->rotStyle == ROTSTYLE_RANDOM ) {
                if( ss_iRand(10) == 2 )
                    p3dRotL[i] = ss_fRand( 0.0f, 10.0f );
                else if( ss_iRand(10) == 2 )
                    p3dRotL[i] = ss_fRand( 90.0f, 135.0f );
            }
        }
    }

    // change rotation table

    // use i to set a frequency for choosing gInvTrig table
    i = 10;

    oldTrig = pac->pTrig;

    switch( pac->rotStyle ) {
        case ROTSTYLE_RANDOM:
            if( pac->demoType == DEMO_VSTRING )
                i = 7;
            // fall thru...
        case ROTSTYLE_SEESAW:
            // Use InvTrig table every now and then
            if( ss_iRand(i) == 2 ) {
                pac->pTrig = gInvTrig;
            }
            else
                pac->pTrig = gTrig;
            break;
        default:
            // Always use regular trig table
            pac->pTrig = gTrig;
    }

    // if trig table changed, need to adjust steps of non-zero axis rotations
    // (otherwise get 'twitch' in rotation)

    if( pac->pTrig != oldTrig ) {
        // only deal with axis which didn't have amplitudes modified
        for( i = 0; i < NUM_AXIS; i++ )
            reset[i] = ! reset[i];
        AdjustRotationStep( pac, reset, oldTrig );
    }
}

/**************************************************************************\
* AdjustRotationStep
*
* If trig table is changed in ResetRotationLimits, then axis with non-zero
* rotations will appear to jump.  This routine modifies the current step
* so this will not be apparent.
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static void
AdjustRotationStep( AttrContext *pac, int *reset, POINTFLOAT *oldTrig )
{
    int *step = (int *) &pac->ip3dRoti;
    FLOAT *p3dRotMax =  (FLOAT *) &pac->p3dRotMax;
    int axis;
    POINT *trigDif;

    if( pac->demoType == DEMO_VSTRING )
        // for now doesn't matter, string is invisible at this point
        return;

    // choose diff table to use for modifying step
    trigDif = (oldTrig == gTrig) ? gTrigDif : gInvTrigDif;

    for( axis = 0; axis < NUM_AXIS; axis++ ) {
        if( p3dRotMax[axis] && reset[axis] ) {
            if( axis != Z_AXIS )
                step[axis] += trigDif[ step[axis] ].x;
            else
                step[axis] += trigDif[ step[axis] ].y;

            // check for wrap or out of bounds
            if( (step[axis] >= 360) || (step[axis] < 0) )
                step[axis] = 0;
        }
    }
}

/**************************************************************************\
* FindInvStep
*
* Finds step in invTrig table with same value as trig table at i
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static int
FindInvStep( int i )
{
    FLOAT val, diff, minDiff;
    int invStep = i;

    val = gTrig[i].y;

    invStep = i;
    minDiff = val - gInvTrig[i].y;

    while( ++i <= 90 ) {
        diff = val - gInvTrig[i].y;
        if( (FLOAT) fabs(diff) < minDiff ) {
            minDiff = (FLOAT) fabs(diff);
            invStep = i;
        }
        if( diff < 0.0f )
            break;
    }

    return invStep;
}


/**************************************************************************\
* FindStep
*
* Finds step in trig table with same value as invTrig table at i
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static int
FindStep( int i )
{
    FLOAT val, diff, minDiff;
    int step = i;

    val = gInvTrig[i].y;

    step = i;
    minDiff = gTrig[i].y - val;

    while( --i >= 0 ) {
        diff = val - gTrig[i].y;
        if( (FLOAT) fabs(diff) < minDiff ) {
            minDiff = (FLOAT) fabs(diff);
            step = i;
        }
        if( diff > 0.0f )
            break;
    }

    return step;
}

/**************************************************************************\
* InitTrigTable
*
* Initialize trig look-up tables
*
* History :
*  Apr. 28, 95 : [marcfo]
*    - Calculate 'diff' tables for smooth transitions between trig and
*      invTrig tables
*
\**************************************************************************/
static void
InitTrigTable()
{
    int i;
    static int num = 360;
    FLOAT inc = (2.0f*PI)/((FLOAT)num);  // 360 degree range
    FLOAT angle = 0.0f;
    int newStep;

    // calc standard trig table

    for( i = 0; i < num; i ++ ) {
        gTrig[i].x = (FLOAT) cos(angle);
        gTrig[i].y = (FLOAT) sin(angle);
        angle += inc;
    }

    // Calc sawtooth and pseudo-inverse trig table, as well as a diff
    // table to convert between trig and invTrig.

    // do y, or sin values first

    for( i = 0; i <= 90; i ++ ) {
        gSawTooth[i].y = (int) i / 90.0f;
        gInvTrig[i].y = 2*gSawTooth[i].y - gTrig[i].y;
    }

    // Create tables to convert trig steps to invTrig steps, and vice-versa
    for( i = 0; i <= 90; i ++ ) {
        newStep = FindInvStep( i );
        gTrigDif[i].y = newStep - i;
        newStep = FindStep( i );
        gInvTrigDif[i].y = newStep - i; // -
    }

    // reflect 0-90 to get 90-180
    for( i = 1; i <= 90; i ++ ) {
        gSawTooth[90+i].y = gSawTooth[90-i].y;
        gInvTrig[90+i].y = gInvTrig[90-i].y;
        gTrigDif[90+i].y = -gTrigDif[90-i].y;
        gInvTrigDif[90+i].y = -gInvTrigDif[90-i].y;
    }
    // invert 0-180 to get 180-360
    for( i = 1; i < 180; i ++ ) {
        gSawTooth[180+i].y = -gSawTooth[i].y;
        gInvTrig[180+i].y = -gInvTrig[i].y;
        gTrigDif[180+i].y = gTrigDif[i].y;
        gInvTrigDif[180+i].y = gInvTrigDif[i].y;
    }

    // calc x, or cos, by phase-shifting y

    for( i = 0; i < 270; i ++ ) {
        gSawTooth[i].x = gSawTooth[i+90].y;
        gInvTrig[i].x = gInvTrig[i+90].y;
        gTrigDif[i].x = gTrigDif[i+90].y;
        gInvTrigDif[i].x = gInvTrigDif[i+90].y;
    }
    for( i = 0; i < 90; i ++ ) {
        gSawTooth[i+270].x = gSawTooth[i].y;
        gInvTrig[i+270].x = gInvTrig[i].y;
        gTrigDif[i+270].x = gTrigDif[i].y;
        gInvTrigDif[i+270].x = gInvTrigDif[i].y;
    }
}

/**************************************************************************\
* CalcChordalDeviation
*
*
\**************************************************************************/
static FLOAT
CalcChordalDeviation( HDC hdc, AttrContext *pac )
{
    OUTLINETEXTMETRIC otm;
    FLOAT       cd, mincd;  // chordal deviations

    // Query font metrics

    if( GetOutlineTextMetrics( hdc, sizeof(otm), &otm) <= 0 )
        // cmd failed, or buffer size=0
        return 1.0f;

    // minimum chordal deviation is limited by design space
    mincd = 1.0f / (FLOAT) otm.otmEMSquare;

    // now map fTesselFact to chordalDeviation
    cd = MapValue( pac->fTesselFact,
                   0.0f, 1.0f,  // fTesselFact range
                   FMAX_CHORDAL_DEVIATION, mincd );  // chordalDeviation range
    if( pac->fTesselFact == 0.0f )
        // make sure get lowest resolution
        cd = 1.0f;
    return cd;
}

/**************************************************************************\
* CalcBoundingBox
*
\**************************************************************************/
static void
CalcBoundingBox( AttrContext *pac )
{
    POINT3D box[3];  // for each axis rotation
    FLOAT viewAngle, critAngle, critAngleC, rectAngle;
    FLOAT r, rot, x, y, z, xmax, ymax, zCrit;
    FLOAT viewDist, viewDistO, xAngle[3], angle;
    FLOAT boxvpo[3];  // viewpoint to origin distance along z for the boxes
    int n = 0;
    POINTFLOAT extent;
    POINT3D pt;

    /* One thing to remember here is that box[n].z is constrained to be
     * the near clipping plane.  The boxe's x and y represent the frustum
     * cross-section at that point.
     */
    viewAngle = deg_to_rad( pac->fFovy )  / 2.0f;

    // x,y,z represent half-extents
    x = pac->pfTextExtent.x/2.0f;
    y = pac->pfTextExtent.y/2.0f;
    z = pac->fDepth/2.0f;
    // initialize box[0] with current extents
    box[0].x = x;
    box[0].y = y;
    box[0].z = z;
    boxvpo[0] = 0.0f;

    // handle rotation around x-axis

    if( pac->p3dRotMax.x != 0.0f ) {

        box[n].x = x;

        // need to determine y and z

        rot = deg_to_rad( pac->p3dRotMax.x );
        r = (FLOAT) sqrt( y*y + z*z );

        // calc incursion along z

        rectAngle = (z == 0.0f) ? PI_OVER_2 : (FLOAT) atan( y/z );
        if( rot >= rectAngle ) {
            // easy, use maximum possible extent
            box[n].z = r;
        } else {
            // rotate lower right corner of box by rot to get extent
            box[n].z = z * (FLOAT) cos( rot ) + y * (FLOAT) sin( rot );
        }

        /* figure out critical angle, where rotated rectangle would
         * be perpendicular to viewing frustum.  This indicates the max.
         * y-incursion into the frustum.
         */
        critAngle = PI_OVER_2 - viewAngle;
        ymax = r * (FLOAT) sin(critAngle);

        if( y > z ) {
            rectAngle = PI_OVER_2 - rectAngle;
            critAngleC = PI_OVER_2 - critAngle;
        } else
            critAngleC = critAngle;

        if( (rectAngle + rot) >= critAngleC ) {
            // no view reduction possible in y, use max view
            // need to calc y at box.z
            viewDistO = r / (FLOAT) cos( critAngle );
            boxvpo[n] = viewDistO;
            box[n].y = (viewDistO - box[n].z) *
                                        (FLOAT) tan( PI_OVER_2 - critAngle);
        } else {
            // we can sonic reduce it
            if( y > z )
                rot = -rot;
            // rotate front-top point by rot to get ymax, z
            ymax = z * (FLOAT) sin( rot ) + y * (FLOAT) cos( rot );
            zCrit = z * (FLOAT) cos( rot ) - y * (FLOAT) sin( rot );
            // not usin viewDistO properly here...
            viewDistO = ymax * (FLOAT) tan( viewAngle);
            boxvpo[n] = viewDistO + zCrit;
            viewDist = boxvpo[n] - box[n].z;
            box[n].y = viewDist * (FLOAT) tan( viewAngle );
        }
        n++;
    }

    if( pac->p3dRotMax.y != 0.0f ) {

        box[n].y = y;

        // need to determine x and z
        rot = deg_to_rad( pac->p3dRotMax.y );
        r = (FLOAT) sqrt( x*x + z*z );
        rectAngle = (z == 0.0f) ? PI_OVER_2 : (FLOAT) atan( x/z );

        // calc incursion along z

        if( rot >= rectAngle ) {
            // easy, use maximum possible extent
            box[n].z = r;
        } else {
            // rotate lower right corner of box by rot to get extent
            box[n].z = z * (FLOAT) cos( rot ) + x * (FLOAT) sin( rot );
        }

        // view distance to largest z
        viewDist = y / (FLOAT) tan(viewAngle);
        // make viewDist represent distance to origin
        viewDistO = viewDist + box[n].z;
        boxvpo[n] = viewDistO;

        // now minimize angle between viewpoint and rotated rect

        if( viewDistO > r ) {
            /* calc crit angle where view is maximized (line from viewpoint
             * tangent to rotation circle)
             * critAngle is between z-axis and radial line
             */
            critAngle = (FLOAT) acos( r / viewDistO );

            // critAngleC is for Comparing
            if( x > z ) {
                rectAngle = PI_OVER_2 - rectAngle;
                critAngleC = PI_OVER_2 - critAngle;
            } else
                critAngleC = critAngle;
        }

        if( (viewDistO > r) && // vp OUTSIDE circle
            ((rectAngle + rot) >= critAngleC) ) {

            /* no view reduction possible in x, use x along the max-view line
             */
            box[n].x = viewDist * (FLOAT) tan( PI_OVER_2 - critAngle );
        } else {
            // we can sonic reduce it
            if( x > z )
                rot = -rot;
            // rotate front-top point by rot to get x,z
            // pt.z not needed
            //pt.z = z * (FLOAT) cos( rot ) - x * (FLOAT) sin( rot );
            pt.x = z * (FLOAT) sin( rot ) + x * (FLOAT) cos( rot );
            box[n].x = pt.x;
        }
        n++;
    }

    if( pac->p3dRotMax.z != 0.0f ) {

        CalcBoundingExtent( deg_to_rad(pac->p3dRotMax.z),
                            x, y,
                            (POINTFLOAT *) &box[n] );
        box[n].z = z;
        // calc viewing distance from front of box
        viewDist = box[n].y / (FLOAT) tan(viewAngle);
        // calc view distance to origin;
        boxvpo[n] = box[n].z + viewDist;
        n++;
    }

    /* XXX!: this is currently only being used for case of one axis rotation
     * There were clipping problems using it for more axis'
     */

    /* Now we've got 3 rectangles in x-y plane at various depths, and
     * need to pick the shortest viewing distance that will encompass
     * all of them.  Or, don't actually have to pick the view distance
     * yet, since might want to wait for the viewport size in Reshape before
     * we do this - but in that case need to pick the rectangle that 'sticks
     * out the most', so it can be used as the bounding box.
     */
    /* The box with the furthest viewpoint will work for y.
     * But then have to check
     * this against the x's of the others.  If any stick out of the frustum,
     * then it will have to be made larger in x.  By making x larger, we
     * do not affect fovy
     */
    SortBoxes( box, boxvpo, n ); // put largest viewpoint box in box[0]

    // figure view dist to first box
    // (could maintain these as they are calculated)
    viewDist = boxvpo[0] - box[0].z;

    // compare x angles of boxes

    switch( n ) {
        FLOAT den;

        case 3:
            den = viewDist + (box[0].z - box[2].z);
            xAngle[2] = den == 0.0f ? PI_OVER_2 : (FLOAT)atan( box[2].x / den);
        case 2:
            den = viewDist + (box[0].z - box[1].z);
            xAngle[1] = den == 0.0f ? PI_OVER_2 : (FLOAT)atan( box[1].x / den);
        case 1:
            xAngle[0] = viewDist == 0.0f ? PI_OVER_2 :
                                            (FLOAT)atan( box[0].x / viewDist );
    }

    // here, just call Sort again, with list of xAngles
    SortBoxes( box, xAngle, n ); // put largest xangle box in box[0]

    // now box[0] should contain half extents of the Bounding box

    pac->p3dBoundingBox = box[0];
}

/**************************************************************************\
* CalcBoundingBoxFromSphere
*
* Calculates the bounding box from a sphere with r = diagonal of the box
*
\**************************************************************************/
static void
CalcBoundingBoxFromSphere( AttrContext *pac )
{
    FLOAT x, y, z, r;
    FLOAT viewAngle, viewDist, viewDistO;
    POINT3D box;

    // x,y,z represent half-extents
    x = pac->pfTextExtent.x/2.0f;
    y = pac->pfTextExtent.y/2.0f;
    z = pac->fDepth/2.0f;

    r = (FLOAT) sqrt( x*x + y*y +z*z );
    box.z = r;
    viewAngle = deg_to_rad( pac->fFovy )  / 2.0f;
    viewDistO = r / (FLOAT) sin( viewAngle );
    viewDist = viewDistO - r;
    box.y = viewDist * (FLOAT) tan( viewAngle );
    box.x = box.y;

    pac->p3dBoundingBox = box;
}

/**************************************************************************\
*
* CalcBoundingBoxFromSpherePlus
*
* Same as above, but tries to optimize for case when z exent is small
*
\**************************************************************************/
static void
CalcBoundingBoxFromSpherePlus( AttrContext *pac, FLOAT zmax )
{
    FLOAT x, y, z, r;
    FLOAT viewAngle, viewDist, viewDistO;
    POINT3D box;

    // x,y,z represent half-extents
    x = pac->pfTextExtent.x/2.0f;
    y = pac->pfTextExtent.y/2.0f;
    z = pac->fDepth/2.0f;

    r = (FLOAT) sqrt( x*x + y*y +z*z );
    viewAngle = deg_to_rad( pac->fFovy )  / 2.0f;

    if( zmax < r ) {
        // we can get closer !
        box.z = zmax;
        viewDistO = r / (FLOAT) sin( viewAngle );
        viewDist = viewDistO - zmax;

        // we want to move the clipping plane closer by (r-zmax)
        if( (r-zmax) > viewDist ) {
#ifdef SS_DEBUG
            glClearColor( 1.0f, 0.0f, 0.0f, 0.0f );
#endif
            // we are moving the vp inside the sphere
            box.y = (FLOAT) sqrt( r*r - box.z*box.z );
            box.x = box.y;
        } else {
            FLOAT zt; // z-point where view frustum tangent to sphere

            // vp outside sphere: can only optimize if zmax < ztangent
            zt = r * (FLOAT) cos( PI_OVER_2 - viewAngle);
            if( zmax < zt ) {
#ifdef SS_DEBUG
                // GREEN ZONE !!!
                glClearColor( 0.0f, 1.0f, 0.0f, 0.0f );
#endif
                box.y = (FLOAT) sqrt( r*r - zmax*zmax );
            } else
                // this is the same as below, but with better clipping
                box.y = (viewDist + (r-zmax)) * (FLOAT) tan( viewAngle );
            box.x = box.y;
        }
    } else {
        box.z = r;
        viewDistO = r / (FLOAT) sin( viewAngle );
        viewDist = viewDistO - r;
        box.y = viewDist * (FLOAT) tan( viewAngle );
        box.x = box.y;
    }
    pac->p3dBoundingBox = box;
}

/**************************************************************************\
* CalcBoundingBoxFromExtents
*
* Calculate bounding box for text, assuming text centered at origin, and
* using maximum possible spin angles.
*
* Rotation around any one axis will affect bounding areas in the other
* 2 directions (e.g. z-rotation affects x and y bounding values).
*
* We need to find the maxima of the rotated 2d area, while staying within
* the max spin angles.
*
\**************************************************************************/
static void
CalcBoundingBoxFromExtents( AttrContext *pac, POINT3D *box )
{
    POINTFLOAT extent;

    box->x = pac->pfTextExtent.x / 2.0f;
    box->y = pac->pfTextExtent.y / 2.0f;
    box->z = pac->fDepth / 2.0f;

    // split the 3d problem into 3 2d problems in 'x-y' plane

    if( pac->p3dRotMax.x != 0.0f ) {

        CalcBoundingExtent( deg_to_rad(pac->p3dRotMax.x),
                            box->z, box->y, &extent );
        box->z = max( box->z, extent.x );
        box->y = max( box->y, extent.y );
    }

    if( pac->p3dRotMax.y != 0.0f ) {

        CalcBoundingExtent( deg_to_rad(pac->p3dRotMax.y),
                            box->x, box->z, &extent );
        box->x = max( box->x, extent.x );
        box->z = max( box->z, extent.y );
    }

    if( pac->p3dRotMax.z != 0.0f ) {

        CalcBoundingExtent( deg_to_rad(pac->p3dRotMax.z),
                            box->x, box->y, &extent );
        box->x = max( box->x, extent.x );
        box->y = max( box->y, extent.y );
    }
}

/**************************************************************************\
* CalcBoundingBoxGeneric
*
* Combines the bounding sphere with the bounding extents
* Each of these alone will guarantee no clipping.  But we can
* optimize by combining them.
*
\**************************************************************************/
static void
CalcBoundingBoxGeneric( AttrContext *pac )
{
    POINT3D extentBox;
    FLOAT x, y, z, r, d, zt, fovx;
    FLOAT viewAngle, viewDist, viewDistO;
    BOOL xIn, yIn;

    // x,y,z represent half-extents
    x = pac->pfTextExtent.x/2.0f;
    y = pac->pfTextExtent.y/2.0f;
    z = pac->fDepth/2.0f;


    // get the max extent box

    /*!!! wait, this alone doesn't guarantee no clipping?  It only
     * checks each axis-rotation separately, without combining them. This
     * is no better than calling old CalcBoundingBox ... ??  Well, I
     * can't prove why theoretically, but it works
     */
    CalcBoundingBoxFromExtents( pac, &extentBox );

    // determine whether x and y extents inside/outside bounding sphere

    r = (FLOAT) sqrt( x*x + y*y +z*z );
    // check y
    d = (FLOAT) sqrt( extentBox.y*extentBox.y + extentBox.z*extentBox.z );
    yIn = d <= r ? TRUE : FALSE;
    // check x
    d = (FLOAT) sqrt( extentBox.x*extentBox.x + extentBox.z*extentBox.z );
    xIn = d <= r ? TRUE : FALSE;

    // handle easy cases

    if( yIn && xIn ) {
        pac->p3dBoundingBox = extentBox;
        return;
    }
    if( !yIn && !xIn ) {
        CalcBoundingBoxFromSpherePlus( pac, extentBox.z );
        return;
    }

    // harder cases

    viewAngle = deg_to_rad( pac->fFovy )  / 2.0f;

    if( yIn ) {
        // figure out x
        viewDist = extentBox.y / (FLOAT) tan(viewAngle);
        /* viewDist can be inside or outside of the sphere
         * If inside - no optimization possible
         * If outside, can draw line from viewpoint tangent to sphere,
         * and use this point for x
         */
        viewDistO = extentBox.z + viewDist;
        if( viewDistO <= r ) {
            // vp inside sphere
            // set x to the point where z intersects sphere
            // this becomes a Pythagorous theorem problem:
            extentBox.x = (FLOAT) sqrt( r*r - extentBox.z*extentBox.z );
        } else {
            // vp outside sphere
            /* - figure out zt, where line tangent to circle for viewAngle
             */
            fovx = (FLOAT) asin( r / viewDistO );
            zt = r * (FLOAT) acos( PI_OVER_2 - viewAngle);
            if( extentBox.z < zt ) {
                // use x where extentBox.z intersects sphere
                extentBox.x = (FLOAT) sqrt( r*r - extentBox.z*extentBox.z );
            } else {
                // use x at tangent point
                extentBox.x = (FLOAT) sqrt( r*r - zt*zt );
            }
        }
    } else {// y out, x in
        // XXX!
        // have to figure out whether vp inside/outside of sphere.
        /* !We can cheat a bit here.  It IS possible, with view angles > 90,
         * that the vp be inside sphere.  But since we always use 90 for
         * this app, it is safe to assume vp > r  (Fix later for general case)
         */
        // XXX: wait, if y out, isn't vp always outside sphere ?
        /* So we solve it this way:
         * - figure out line tangent to circle for viewAngle
         * - y will be where this line intersects the z=extentBox.z line
         */
        viewDistO = r / (FLOAT) sin( viewAngle );
        extentBox.y = (viewDistO - extentBox.z) * (FLOAT) tan( viewAngle );
        // I guess don't have to do anything with x ?
    }
    pac->p3dBoundingBox = extentBox;
}

/**************************************************************************\
*
* Calculate the extents in x and y from rotating a rectangle in a 2d plane
*
\**************************************************************************/
static void
CalcBoundingExtent( FLOAT rot, FLOAT x, FLOAT y, POINTFLOAT *extent )
{
    FLOAT r, angleCrit;

    r = (FLOAT) sqrt( x*x + y*y );
    angleCrit = (x == 0.0f) ? PI_OVER_2 : (FLOAT) atan( y/x );

    // calc incursion in x

    if( rot >= angleCrit ) {
        // easy, use maximum possible extent
        extent->x = r;
    } else {
        // rotate lower right corner of box by rot to get extent
        extent->x = x * (FLOAT) cos( rot ) + y * (FLOAT) sin( rot );
    }

    // calc incursion in y

    angleCrit = PI/2.0f - angleCrit;

    if( rot >= angleCrit ) {
        // easy, use maximum possible extent
        extent->y = r;
    } else {
        // rotate upper right corner of box by rot to get extent
        extent->y = x * (FLOAT) sin( rot ) + y * (FLOAT) cos( rot );
    }

}

/**************************************************************************\
*
* Sorts in descending order, based on values in val array (bubble sort)
*
\**************************************************************************/
static void
SortBoxes( POINT3D *box, FLOAT *val, int numBox )
{
    int i, j, t;
    POINT3D temp;

    j = numBox;
    while( j ) {
        t = 0;
        for( i = 0; i < j-1; i++ ) {
            if( val[i] < val[i+1] ) {
                // swap'em
                temp = box[i];
                box[i] = box[i+1];
                box[i+1] = temp;
                t = i;
            }
        }
        j = t;
    }
}

#define FILE_BUF_SIZE 180

/**************************************************************************\
* VerifyString
*
* Validate the string
*
* Has hard-coded ascii routines
*
\**************************************************************************/
static BOOL
VerifyString( AttrContext *pac )
{
    HMODULE ghmodule;
    HRSRC hr;
    HGLOBAL hg;
    PSZ psz, pszFile = NULL;
    CHAR szSectName[30], szFileName[FILE_BUF_SIZE], szFname[30];
    BOOL bMatch = FALSE;

    // Check for string file in registry
    if (LoadStringA(hMainInstance, IDS_SAVERNAME, szSectName, 30) &&
        LoadStringA(hMainInstance, IDS_INIFILE, szFname, 30))
    {
        if( GetPrivateProfileStringA(szSectName, "magic", NULL,
                                     szFileName, FILE_BUF_SIZE, szFname) )
            pszFile = ReadStringFileA( szFileName );
    }

    // Check for key strings
    if( pszFile )
        bMatch = CheckKeyStrings( pac->szText, pszFile );

    if( !bMatch ) {
        if( (ghmodule = GetModuleHandle(NULL)) &&
            (hr = FindResource(ghmodule, MAKEINTRESOURCE(1), 
                                                    MAKEINTRESOURCE(99))) &&
            (hg = LoadResource(ghmodule, hr)) &&
            (psz = (PSZ)LockResource(hg)) )
        bMatch = CheckKeyStrings( pac->szText, psz );
    }

    if( bMatch ) {
        // put first string in pac->szText
        pac->demoType = DEMO_VSTRING;
        // for now, initialize strings here
        text3d_UpdateString( pac, FALSE );

        // adjust cycle time based on rotStyle
        switch( pac->rotStyle ) {
            case ROTSTYLE_NONE:
                gfMinCycleTime = 4.0f;
                break;
            case ROTSTYLE_SEESAW:
                gfMinCycleTime = 8.0f;
                break;
            case ROTSTYLE_RANDOM:
                gfMinCycleTime = 10.0f;
                break;
            default:
                gfMinCycleTime = 9.0f;
        }
    }
    if( pszFile )
        free( pszFile );  // allocated by ReadStringFile
    return bMatch;
}

/**************************************************************************\
* CheckKeyStrings
*
* Test for match between string and any keystrings
* 'string' is user-inputted, and limited to TEXT_LIMIT chars.
*
\**************************************************************************/

static BOOL
CheckKeyStrings( LPTSTR string, PSZ psz )
{
    int i;
    TCHAR szKey[TEXT_LIMIT+1], testString[TEXT_LIMIT+1] = {0};
    BOOL bMatch = FALSE;
    int nMatch = 0;
    int len;

    // make copy of test string and convert to upper case
    lstrcpy( testString, string );
#ifdef UNICODE
    _wcsupr( testString );
#else
    _strupr( testString );
#endif

    while( psz[0] != '\n' ) { // iterate keyword/data sets
        while( psz[0] != '\n' ) {  // iterate keywords
            len = strlen( psz ); // ! could be > TEXT_LIMIT if from file
            // invert keyword bits and convert to uppercase
#ifdef UNICODE
            // convert ascii keyword to unicode in szKey (inverts at same time)
            ConvertStringAsciiToUnicode( psz, szKey,
                                         len > TEXT_LIMIT ? TEXT_LIMIT : len );
            _wcsupr( szKey );
#else
            // just copy keyword to szKey, without going over TEXT_LIMIT
            strncpy( szKey, psz, TEXT_LIMIT );
            InvertBitsA( szKey, len > TEXT_LIMIT ? TEXT_LIMIT : len );
            szKey[TEXT_LIMIT] = '\0';  // in case len > TEXT_LIMIT
            _strupr( szKey );
#endif

            if( !lstrcmp( szKey, testString ) ) {
                // keyword match !
                bMatch = TRUE;
                nMatch++;
            }
            psz += len + 1;  // skip over NULL as well
        }
        psz++;  // skip over '\n' at end of keywords
        if( bMatch )
            ReadNameList( psz );

        // skip over data to get to next keyword group
        while( *psz != '\n' )
            psz++;
        psz++;  // skip over '\n' at end of data
        bMatch = FALSE; // keep searching for keyword matches
    }
    return nMatch;
}

/**************************************************************************\
* Various functions to process vstrings
*
\**************************************************************************/

static void
InvertBitsA( char *s, int len )
{
    while( len-- ) {
        *s++ = ~(*s);
    }
}

static PSZ
ReadStringFileA( LPSTR szFile )
{
    char lineBuf[180];
    PSZ buf, pBuf;
    int size, length, fdi;
    char *ps;
    char ctrl_n = '\n';
    FILE *fIn;
    BOOL bKey;

    // create buffer to hold entire file
    // mf: ! must be better way of getting file length!
    fdi = _open(szFile, O_RDONLY | O_BINARY);
    if( fdi < 0 )
        return NULL;
    size = _filelength( fdi );
    _close(fdi);
    buf= (char *) malloc( size );
    if( !buf)
        return NULL;

    // open file for ascii text reading
    fIn = fopen( szFile, "r" );
    if( !fIn )
        return NULL;

    // Read in keyword/data sequences

    bKey = TRUE;  // so '\n' not appended to file when hit first keyword
    pBuf = buf;
    while( fgets( lineBuf, 180, fIn) ) {
        ps = lineBuf;
        if( *ps == '-' ) {
            // keyword
            if( !bKey ) {
                // first key in group, append '\n' to data
                *pBuf++ = ctrl_n;
            }
            bKey = TRUE;
            ps++; // skip '-'
        } else {
            // data
            if( bKey ) {
                // first data in group, append '\n' to keywords
                *pBuf++ = ctrl_n;
            }
            bKey = FALSE;
        }
        length = strlen( ps );
        InvertBitsA( ps, length );
        *(ps+length-1) = '\0'; // convert '\n' to null
        lstrcpyA( pBuf, ps );
        pBuf += length;
    }
    fclose( fIn );

    // put 2 '\n' at end, for end condition
    *pBuf++ = ctrl_n;
    *pBuf++ = ctrl_n;
    return( buf );
}

static void
CreateRandomList()
{
   PLIST plist = gplistComplete;
   PLIST *pplist;
   int i = 0;
   int n;

   while (plist != NULL) {
       n = ss_iRand( i+1 );
       pplist = &gplist;

       while (n > 0) {
           pplist = &((*pplist)->pnext);
           n--;
       }

       plist->pnext = *pplist;
       *pplist = plist;

       plist = plist->plistComplete;
       i++;
   }
}

static void
AddName(
    LPTSTR pszStr)
{
    PLIST plist = (PLIST)LocalAlloc(LPTR, sizeof(LIST));
    if( !plist )
        return;
    plist->pszStr = pszStr;
    plist->pnext = NULL;
    plist->plistComplete = gplistComplete;
    gplistComplete = plist;
}

static void
ReadNameList( PSZ psz )
{
    int length;
    int i;
    LPTSTR pszNew;

    while (psz[0] != '\n') {
        length = 0;
        while (psz[length] != 0) {
            length++;
        }
        length;
        pszNew = (LPTSTR)LocalAlloc( LPTR, (length + 1)*sizeof(TCHAR) );
        if( !pszNew )
            return;
#ifdef UNICODE
        ConvertStringAsciiToUnicode( psz, (PWSTR) pszNew, length );
#else
        strncpy( pszNew, psz, length );
        InvertBitsA( pszNew, length );
#endif
        AddName(pszNew);

        psz += length + 1;
    }
}

static void
DeleteNameList()
{
    PLIST plist = gplistComplete, plistLast;

    while( plist != NULL ) {
        LocalFree( plist->pszStr );
        plistLast = plist;
        plist = plist->plistComplete;
        LocalFree( plistLast );
    }
}

/**************************************************************************\
* ConvertStringAsciiToUnicorn
*
\**************************************************************************/
static void
ConvertStringAsciiToUnicode( PSZ psz, PWSTR pwstr, int len )
{
    while( len-- )
         *pwstr++ = ~(*psz++) & 0xFF;
    *pwstr = 0; // null terminate
}

/**************************************************************************\
* FrameCalibration
*
* Adjusts the number of frames in a cycle to conform to desired cycle time
*
\**************************************************************************/
static int
FrameCalibration( AttrContext *pac, struct _timeb *pBaseTime, int framesPerCycle, int nCycle )
{
    struct _timeb thisTime;
    FLOAT cycleTime;

    _ftime( &thisTime );
    cycleTime = thisTime.time - pBaseTime->time +
           (thisTime.millitm - pBaseTime->millitm)/1000.0f;
    cycleTime /= (FLOAT) nCycle;

    if( cycleTime < gfMinCycleTime ) {
        // need to add more frames to cycle
        if( cycleTime == 0.0f ) // very unlikely
            framesPerCycle = 800;
        else
            framesPerCycle = (int)( (FLOAT)framesPerCycle *
                            (gfMinCycleTime/cycleTime) );
    } else {
        // for vstrings, subtract frames from cycle
        if( pac->demoType == DEMO_VSTRING ) {
            framesPerCycle = (int)( (FLOAT)framesPerCycle *
                            (gfMinCycleTime/cycleTime) );
        }
    }
#define MIN_FRAMES 16
    // make sure it's not too small
    if( framesPerCycle < MIN_FRAMES )
        framesPerCycle = MIN_FRAMES;

    return framesPerCycle;
}

/**************************************************************************\
* MapValue
*
* Maps the value along an input range, to a proportional one along an
* output range.  Each range must be monotonically increasing or decreasing.
*
* NO boundary conditions checked - responsibility of caller.
*
\**************************************************************************/
FLOAT
MapValue( FLOAT fInVal,
          FLOAT fIn1, FLOAT fIn2,       // input range
          FLOAT fOut1, FLOAT fOut2 )    // output range
{
    FLOAT fDist, fOutVal;

    // how far along the input range is fInVal?, in %
    fDist = (fInVal - fIn1) / (fIn2 - fIn1);

    // use this distance to interpolate into output range
    fOutVal = fDist * (fOut2 - fOut1) + fOut1;

    return fOutVal;
}

/**************************************************************************\
* MapValueI
*
* Similar to above, but maps integer values
*
* Currently, only works for increasing ranges
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Added early return for boundary conditions
*
\**************************************************************************/
int
MapValueI( int inVal,
          int in1, int in2,       // input range
          int out1, int out2 )    // output range
{
    int inDiv;
    int outVal;
    FLOAT fScale, fComp;

    if( inVal >= in2 )
        return out2;
    if( inVal <= in1 )
        return out1;

    inDiv = abs(in2 - in1) + 1;
    fScale = (FLOAT) (inDiv-1) / (FLOAT) inDiv;
    fComp = 1.0f + (1.0f / inDiv);

    outVal = (int) MapValue( (FLOAT) inVal * fComp,
                       (FLOAT) in1, (FLOAT) in2 + 0.999f,
                       (FLOAT) out1, (FLOAT) out2 + 0.999f );
    return outVal;
}
