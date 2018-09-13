/******************************Module*Header*******************************\
* Module Name: sscommon.h
*
* Defines and externals for screen saver common shell
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef __sscommon_h__
#define __sscommon_h__

#include <GL\gl.h>
#include <assert.h>
#include "tk.h"
#include "matname.h"  // material names

#ifdef __cplusplus
extern "C" {
#endif

#include "ssdebug.h"

#define FAILURE             0
#define SUCCESS             1

// Maximum texture bitmap dimensions.

#define TEX_WIDTH_MAX   1280
#define TEX_HEIGHT_MAX  1024

#define PI 3.14159265358979323846f
// double version of PI
#define PI_D 3.14159265358979323846264338327950288419716939937510
#define ONE_OVER_PI (1.0f / PI)
#define ROOT_TWO 1.414213562373f

#define GEN_STRING_SIZE 64

// texture quality level
enum {
    TEXQUAL_DEFAULT = 0,
    TEXQUAL_HIGH
};

typedef struct _point2d {
    GLfloat x;
    GLfloat y;
} POINT2D;

typedef struct _ipoint2d {
    int x;
    int y;
} IPOINT2D;

typedef struct _point3d {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} POINT3D;

typedef struct _ipoint3d {
    int x;
    int y;
    int z;
} IPOINT3D;

typedef struct _texpoint2d {
    GLfloat s;
    GLfloat t;
} TEX_POINT2D;

typedef struct _isize {
    int width;
    int height;
} ISIZE;

typedef struct _fsize {
    GLfloat width;
    GLfloat height;
} FSIZE;

typedef struct _glrect {
    int x, y;
    int width, height;
} GLRECT;

// texture data
typedef struct {
    int     width;
    int     height;
    GLenum  format;
    GLsizei components;
    float   origAspectRatio; // original width/height aspect ratio
    unsigned char *data;
    GLuint  texObj;          // texture object
    int     pal_size;
    int     iPalRot;         // current palette rotation (not used yet)
    RGBQUAD *pal;
} TEXTURE, *HTEXTURE;

#ifndef GL_EXT_paletted_texture
#define GL_COLOR_INDEX1_EXT                   0x80E2
#define GL_COLOR_INDEX2_EXT                   0x80E3
#define GL_COLOR_INDEX4_EXT                   0x80E4
#define GL_COLOR_INDEX8_EXT                   0x80E5
#define GL_COLOR_INDEX12_EXT                  0x80E6
#define GL_COLOR_INDEX16_EXT                  0x80E7
typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC)
    (GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
     GLenum type, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC)
    (GLenum target, GLsizei start, GLsizei count, GLenum format,
     GLenum type, GLvoid *data);
#endif

// texture resource

#define RT_RGB          99
#define RT_MYBMP        100
#define RT_A8           101

// texture resource types
enum {
    TEX_UNKNOWN = 0,
    TEX_RGB,
    TEX_BMP,
    TEX_A8
};

typedef struct {
    int     type;
    int     name;
} TEX_RES;

typedef struct _MATRIX {
    GLfloat M[4][4];
} MATRIX;

typedef struct strRGBA {
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
} RGBA;

typedef struct {
    BYTE r;
    BYTE g;
    BYTE b;
} RGB8;

typedef struct {
    BYTE r;
    BYTE g;
    BYTE b;
    BYTE a;
} RGBA8;

// DlgDraw

enum {
    DLG_INTENSITY_LOW = 0,
    DLG_INTENSITY_MID,
    DLG_INTENSITY_HIGH,
};

// Callback function defines
typedef void (CALLBACK* SSINITPROC)( void *);
typedef void (CALLBACK* SSRESHAPEPROC)(int, int, void *);
typedef void (CALLBACK* SSREPAINTPROC)( LPRECT, void *);
typedef void (CALLBACK* SSUPDATEPROC)( void *);
typedef void (CALLBACK* SSFINISHPROC)( void *);
typedef void (CALLBACK* SSFLOATERFAILPROC)(void *);
typedef void (CALLBACK* SSFLOATERBOUNCEPROC)( void *);

// Defines for pixel format (internal use)
#define SS_DOUBLEBUF_BIT    (1 << 0)
#define SS_DEPTH16_BIT      (1 << 1)
#define SS_DEPTH32_BIT      (1 << 2)
#define SS_ALPHA_BIT        (1 << 3)
#define SS_BITMAP_BIT       (1 << 4)
#define SS_NO_SYSTEM_PALETTE_BIT       (1 << 5)
#define SS_GENERIC_UNACCELERATED_BIT   (1 << 6)

#define SS_HAS_DOUBLEBUF(x) ((x) & SS_DOUBLEBUF_BIT)
#define SS_HAS_DEPTH16(x)	((x) & SS_DEPTH16_BIT)
#define SS_HAS_DEPTH32(x)	((x) & SS_DEPTH32_BIT)
#define SS_HAS_ALPHA(x)     ((x) & SS_ALPHA_BIT)
#define SS_HAS_BITMAP(x)    ((x) & SS_BITMAP_BIT)

// Depth types
enum {
    SS_DEPTH_NONE = 0,
    SS_DEPTH16,
    SS_DEPTH32,
};

// ss context structures

// Note: all *_INFO structures are requests between common and client ss

typedef struct {
    POINT2D    posInc;          // base position increment
    POINT2D    posIncVary;      // +/- variation for posInc on bounce
} MOTION_INFO;

// internal motion structure
typedef struct {
    POINT2D    pos;             // position
    POINT2D    posInc;          // base position increment
    POINT2D    posIncVary;      // +/- variation for posInc on bounce
    POINT2D    posIncCur;       // Current position increment
} MOTION;

// Size and motion attributes of the OpenGL window that floats around.
typedef struct {
    ISIZE      size;
    IPOINT2D    pos;             // position
    MOTION_INFO     motionInfo;
} CHILD_INFO;

typedef void (CALLBACK* SSCHILDSIZEPROC)( ISIZE *, CHILD_INFO * );

// Size and motion attributes of the OpenGL window that floats around.
typedef struct {
    BOOL       bMotion;         // if floater should move or not
    BOOL       bSubWindow;      // If floater is logical sub-window
    SSCHILDSIZEPROC   ChildSizeFunc; // Callback to set size based on parent
} FLOATER_INFO;


// SS_BITMAP already defined in winuser.h
#if defined( SS_BITMAP )
#undef SS_BITMAP
#endif

typedef struct {
    HDC     hdc;
    ISIZE   size;
    HBITMAP hbm;
    HBITMAP hbmOld; // ? necessary ?
} SS_BITMAP;


// Attributes for StretchBlt mode
typedef struct {
    BOOL       bRatioMode;      // use ratio or base width
    float      widthRatio;      // e.g. 2 means use base of (window width / 2)
    float      heightRatio;
    int        baseWidth;
    int        baseHeight;
    SS_BITMAP  ssbm;
} STRETCH_INFO;


typedef struct {
    BOOL bFloater;
    FLOATER_INFO floaterInfo;
    BOOL bStretch;
    STRETCH_INFO stretchInfo;
    BOOL bDoubleBuf;
    int  depthType;
} SSContext, SSC, *PSSC;

typedef struct _MATERIAL {
    RGBA ka;
    RGBA kd;
    RGBA ks;
    GLfloat specExp;
} MATERIAL;

// texture file info

typedef struct {
    int     nOffset;  // filename offset into pathname
    TCHAR   szPathName[MAX_PATH];  // texture pathname
} TEXFILE;

// texture file processing messages

typedef struct {
    TCHAR   szWarningMsg[MAX_PATH];
    TCHAR   szBitmapSizeMsg[MAX_PATH];
    TCHAR   szBitmapInvalidMsg[MAX_PATH];
    TCHAR   szSelectAnotherBitmapMsg[MAX_PATH];
    TCHAR   szTextureDialogTitle[GEN_STRING_SIZE];
    TCHAR   szTextureFilter[2*GEN_STRING_SIZE];
    TCHAR   szBmp[GEN_STRING_SIZE];
    TCHAR   szDotBmp[GEN_STRING_SIZE];
} TEX_STRINGS;

// Resource constants common to all screen savers

#define IDS_COPYRIGHT           9001
#define IDS_GENNAME             9003
#define IDS_INIFILE             9006
#define IDS_HELPFILE            9009
#define IDS_WARNING             9014
#define IDS_ERROR               9015
#define IDS_BITMAP_SIZE         9016
#define IDS_BITMAP_INVALID      9017
#define IDS_SELECT_ANOTHER_BITMAP 9018
#define IDS_START_FAILED        9019
#define IDS_TEXTUREFILTER       9021
#define IDS_TEXTUREDIALOGTITLE  9022
#define IDS_BMP                 9023
#define IDS_DOTBMP              9024
#define IDS_STARDOTBMP          9025
#define IDS_RGB                 9030
#define IDS_DOTRGB              9031
#define IDS_STARDOTRGB          9032
#define IDS_TEXTURE             9126
#define IDS_TEXTURE_FILE_OFFSET 9127
#define IDS_SIZE                9129
#define IDS_TESSELATION         9130

// Useful macros

#define SS_MAX( a, b ) \
    ( a > b ? a : b )

#define SS_MIN( a, b ) \
    ( a < b ? a : b )

// macro to round up floating values
#define SS_ROUND_UP( fval ) \
    ( (((fval) - (FLOAT)(int)(fval)) > 0.0f) ? (int) ((fval)+1.0f) : (int) (fval) )

// macros to clamp a value within a range
#define SS_CLAMP_TO_RANGE( a, lo, hi ) ( (a < lo) ? lo : ((a > hi) ? hi : a) )
#define SS_CLAMP_TO_RANGE2( a, lo, hi ) \
    ( a = (a < lo) ? lo : ((a > hi) ? hi : a) )

// degree<->radian macros
#define ONE_OVER_180 (1.0f / 180.0f)
#define SS_DEG_TO_RAD( a ) ( (a*PI) * ONE_OVER_180 )
#define SS_RAD_TO_DEG( a ) ( (a*180.0f) * ONE_OVER_PI )

extern MATERIAL TeaMaterial[], TexMaterial[], ss_BlackMat;

// window handling

extern void ss_InitFunc(SSINITPROC);
extern void ss_ReshapeFunc(SSRESHAPEPROC);
extern void ss_RepaintFunc(SSREPAINTPROC);
extern void ss_UpdateFunc(SSUPDATEPROC);
extern void ss_FinishFunc(SSFINISHPROC);
extern void ss_FloaterBounceFunc(SSFLOATERBOUNCEPROC);

// This function *must* be defined by the screen saver
extern SSContext* ss_Init(void);
extern BOOL ss_ConfigInit( HWND hDlg );

extern void ss_DataPtr( void * );

extern BOOL ss_SetWindowAspectRatio( FLOAT aspect );
extern void ss_RandomWindowPos( void );

extern HWND ss_GetHWND();
extern HWND ss_GetGLHWND();
extern void ss_GetScreenSize( ISIZE *size );

extern HBITMAP
SSDIB_CreateCompatibleDIB(HDC hdc, HPALETTE hpal, ULONG ulWidth, ULONG ulHeight,
                    PVOID *ppvBits);
extern BOOL APIENTRY SSDIB_UpdateColorTable(HDC hdcMem, HDC hdc, HPALETTE hpal);

// Palette manage procs
extern LONG MainPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LONG PaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LONG FullScreenPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LONG NullPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// material processing

extern void ss_InitTeaMaterials();
extern void ss_InitTexMaterials();
extern void ss_InitMaterials();
extern void ss_SetMaterial( MATERIAL *pMat );
extern void ss_SetMaterialIndex( int index );
extern MATERIAL *ss_RandomTeaMaterial( BOOL bSet );
extern int  ss_RandomTeaMaterialIndex( BOOL bSet );
extern MATERIAL *ss_RandomTexMaterial( BOOL bSet );
extern int  ss_RandomTexMaterialIndex( BOOL bSet );
extern void ss_CreateMaterialGradient( MATERIAL *matInc, MATERIAL *startMat,
                        MATERIAL *endMat, int transCount );
extern void ss_TransitionMaterial( MATERIAL *transMat, MATERIAL *transMatInc );

// color

extern void ss_HsvToRgb(float h, float s, float v, RGBA *color );

// clear

extern int ss_RectWipeClear( int width, int height, int repCount );
extern int ss_DigitalDissolveClear( int width, int height, int size );

// utility

extern void ss_RandInit( void );
extern int ss_iRand( int max );
extern int ss_iRand2( int min, int max );
extern FLOAT ss_fRand( FLOAT min, FLOAT max );
extern BOOL ss_ChangeDisplaySettings( int width, int height, int bitDepth );
extern void ss_QueryDisplaySettings( void );
extern void ss_QueryOSVersion( void );
extern void ss_QueryGLVersion( void );
extern BOOL ss_fOnWin95( void );
extern BOOL ss_fOnNT35( void );
extern BOOL ss_fOnGL11( void );
extern BOOL ss_fPreviewMode( void );
extern BOOL ss_fFullScreenMode( void );
extern BOOL ss_fWindowMode( void );
extern BOOL ss_fConfigMode( void );
extern BOOL ss_RedrawDesktop( void );

// texture file processing

extern int  ss_LoadBMPTextureFile( LPCTSTR pszBmpfile, TEXTURE *pTex );
extern int  ss_LoadTextureFile( TEXFILE *texFile, TEXTURE *pTex );
extern int  ss_LoadTextureResource( TEX_RES *pTexRes, TEXTURE *pTex );
extern BOOL ss_CopyTexture( TEXTURE *pTexDst, TEXTURE *pTexSrc );
extern BOOL ss_SetTextureTransparency( TEXTURE *pTex, float alpha, BOOL bSet );
extern void ss_DisableTextureErrorMsgs();
extern void ss_SetTexture( TEXTURE *pTex );
extern void ss_SetTexturePalette( TEXTURE *pTex, int index );
extern void ss_DeleteTexture( TEXTURE *pTex );
extern BOOL ss_LoadTextureResourceStrings();
extern BOOL ss_DIBImageLoad(PVOID pvFile, TEXTURE *ptex);
extern BOOL ss_RGBImageLoad(PVOID pvFile, TEXTURE *ptex);
extern BOOL ss_A8ImageLoad(PVOID pvFile, TEXTURE *ptex);
extern BOOL ss_VerifyTextureFile( TEXFILE *ptf );
extern BOOL ss_SelectTextureFile( HWND hDlg, TEXFILE *ptf );
extern void ss_GetDefaultBmpFile( LPTSTR pszBmpFile );
extern void ss_InitAutoTexture( TEX_POINT2D *pTexRep );

// texture objects

extern BOOL ss_TextureObjectsEnabled( void );

// Paletted texture support
extern BOOL ss_PalettedTextureEnabled(void);
extern BOOL ss_QueryPalettedTextureEXT(void);

// math functions

extern POINT3D ss_ptZero;
extern void ss_xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void ss_xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void ss_matrixIdent(MATRIX *);
extern void ss_matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta);
extern void ss_matrixTranslate(MATRIX *, double xTrans, double yTrans, double zTrans);
extern void ss_matrixMult( MATRIX *m1, MATRIX *m2, MATRIX *m3 );
extern void ss_calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3);
extern void ss_normalizeNorm(POINT3D *);
extern void ss_normalizeNorms(POINT3D *, ULONG);

// registry functions

extern BOOL ss_RegistrySetup( HINSTANCE hinst, int section, int file );
extern int  ss_GetRegistryInt( int name, int iDefault );
extern void ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, int bufSize );
extern void ss_WriteRegistryInt( int name, int iVal );
extern void ss_WriteRegistryString( int name, LPTSTR lpString );

// dialog helper functions

extern int ss_GetTrackbarPos( HWND hDlg, int item );
extern void ss_SetupTrackbar( HWND hDlg, int item, int lo, int hi, int lineSize, int pageSize, int pos );

extern BOOL gbTextureObjects; // from texture.c

#ifdef __cplusplus
}
#endif

#endif // __sscommon_h__
