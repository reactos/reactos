/******************************Module*Header*******************************\
* Module Name: sstext3d.h
*
* Global header for text3D screen saver.
*
* Created: 12-24-94 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef __sstext3d_h__
#define __sstext3d_h__

#include <commctrl.h>
#include "dlgs.h"
#include "sscommon.h"

#define PI_OVER_2 (PI/2.0f)
#define PI_OVER_4 (PI/4.0f)

#define TEXT_BUF_SIZE       100     // max length of display text buffer
                                    // (including NULL termination)
#define TEXT_LIMIT          16      // max length of user-inputted display text

#define MAX_IROT            100     // max integer rotation level (from slider)

#define MIN_SLIDER          0
#define MAX_SLIDER          100

// demo types
enum {
    DEMO_STRING = 0,    // static string
    DEMO_CLOCK,         // digital clock
    DEMO_VSTRING,       // variable string (actually a subset of DEMO_STRING)
};

#define MAX_DEMO  1             // max demo index
#define NUM_DEMOS (MAX_DEMO+1)

// surface styles
enum {
    SURFSTYLE_SOLID = 0,
    SURFSTYLE_TEX,
    SURFSTYLE_WIREFRAME
};

// rotation styles
enum {
    ROTSTYLE_NONE = 0,
    ROTSTYLE_SEESAW,
    ROTSTYLE_WOBBLE,
    ROTSTYLE_RANDOM,
    NUM_ROTSTYLES
};

enum {
    X_AXIS = 0,
    Y_AXIS,
    Z_AXIS,
    NUM_AXIS
};


#if defined(max)
#undef max
#endif
#define max( a, b ) ( a >= b ? a : b )

#if defined(clamp)
#undef clamp
#endif
#define clamp( a, lo, hi ) ( (a < lo) ? lo : ((a > hi) ? hi : a) )

#define deg_to_rad( a ) ( (a*PI) / 180.0f )
#define rad_to_deg( a ) ( (a*180.0f) / PI )

typedef struct {
    USHORT listNum;             // display list number
    TCHAR     glyph;            // glyph value (for extended LUT entries)
    LPGLYPHMETRICSFLOAT lpgmf;  // ptr to glyphmetrics
} LISTENTRY;

#define SIZE_LIST_LUT  512
#define MAX_DIRECT_LUT 256

typedef struct {
    HDC     hdc;
    int     nGlyphs;            // number of glyphs
    int     firstGlyph;
    FLOAT   chordalDeviation;
    FLOAT   extrusion;
    int     type;               // WGL_FONT_LINES or WGL_FONT_POLYGONS
    LISTENTRY *listLUT;             // LUT for cmd list # from glyph
    int     LUTIndex;           // current index for new indirect look-ups
} WglFontContext;

typedef struct {
    // registry or registry-derived attributes
    int     demoType;
    BOOL    bMaterialCycle;
    int     matType;    // material type from registry
    MATERIAL *pMat;     // ptr to current material
    int     surfStyle;
    int     rotStyle;   // rotation style
    int     texQual;
    float   fTesselFact;
    float   fDepth; // extrusion
    int     iSpeed; // rotation speed
    UINT    uSize;  // window size
    TEXFILE texFile;  // texture file
    TEXTURE texture;
    TCHAR   szFontName[LF_FACESIZE];       // font face name
    BOOL    bBold;
    BOOL    bItalic;
    BYTE    charSet;
    TCHAR   szText[TEXT_BUF_SIZE+1];  // display string
    USHORT  usText[TEXT_BUF_SIZE+1];    // display string converted to cmd lists

    // internal attributes
    BOOL            bTexture;
    BOOL            bRandomMat;
    WglFontContext  *pWglFontC;
    int             textLen;
    POINTFLOAT      pfTextExtent;
    POINTFLOAT      pfTextOrigin;       // upper left corner of extents
    POINT3D         p3dBoundingBox;     // bounding box, from spin angles
    FLOAT           fFovy;              // field of view in y-dir
    FLOAT           fAspect;            // aspect ratio of GL window
    FLOAT           fViewDist;          // dist to front of bounding box
    FLOAT           fZtrans;            // translation in z
    int             iRotStep;           // rotation step from slider
    int             iRotMinStep;        // min rotation step
    int             iRotMaxStep;        // max rotation step
    IPOINT3D        ip3dRotStep;        // xyz rot step, for random rotation
    IPOINT3D        ip3dRoti;           // current xyz rot step iteration
    POINT3D         p3dRotMin;          // min rotation amplitude
    POINT3D         p3dRotMax;          // max rotation amplitude
    POINT3D         p3dRotLimit;        // current rotation amplitude
    POINT3D         p3dRot;             // current rotation
    POINTFLOAT      *pTrig;             // current trig table
    SYSTEMTIME      stTime;
    BOOL            bXMajor;            // string either x-major or y-major

    SSContext       ssc;                // screen saver configuration
} AttrContext;

// Global attribute context
extern AttrContext gac;

extern WglFontContext* CreateWglFontContext ( 
    HDC     hdc, 
    int     type, 
    float   fExtrusion, 
    float   fChordalDeviation );

extern void DeleteWglFontContext( 
    WglFontContext *pwfc );

extern void DrawString ( 
    USHORT *string, 
    int     strLen, 
    WglFontContext *pwfc );

extern int GetStringExtent( 
    LPTSTR  pszString, 
    POINTFLOAT *extent, 
    POINTFLOAT *origin,
    WglFontContext *pwfc );

extern void ConvertStringToList( 
    LPTSTR pszSrc, 
    USHORT *usDst, 
    WglFontContext *pwfc );

extern void getIniSettings(void);


// Resource constants

#define IDS_SCREENSAVERTITLE    1020
#define IDS_SAVERNAME           1002
#define IDS_DEFFONT             1099
#define IDS_DEFCHARSET          1098

// registry attribute strings:
#define IDS_DEMOTYPE            1100
#define IDS_SURFSTYLE           1104
#define IDS_FONT                1109
#define IDS_FONT_ATTRIBUTES     1110
#define IDS_CHARSET             1111
#define IDS_TEXT                1115
#define IDS_SPEED               1120
#define IDS_ROTSTYLE            1124

// demo type strings
#define IDS_DEMO_STRING       1200
#define IDS_DEMO_CLOCK        1201

// rotation resource strings
#define IDS_ROTSTYLE_NONE            1400
#define IDS_ROTSTYLE_SEESAW          1401
#define IDS_ROTSTYLE_WOBBLE          1402
#define IDS_ROTSTYLE_RANDOM          1403

#define DLG_SETUP_HELP          2001
#define DLG_SETUP_TYPES         2002    // object type menu
#define DLG_SETUP_BITMAP        2003
#define DLG_SETUP_ABOUT         2010
#define DLG_SETUP_TESSEL        2012    // tesselation slider
#define DLG_SETUP_SIZE          2014    // size slider
#define DLG_SETUP_TEX           2016    // texture button
#define DLG_SETUP_FONT          2022    // select font button
#define DLG_SETUP_SPEED         2023    // speed slider

// surface styles
#define IDC_RADIO_SOLID         2030
#define IDC_RADIO_TEX           2031
#define IDC_RADIO_WIREFRAME     2032    // not presently used

#define IDC_TO_SURFSTYLE(n)         ( (n) - IDC_RADIO_SOLID )
// In order for the IDC_TO_SURFSTYLE conversion macro to work, the radio buttons
// for surface styles must be kept contiguous.

// rotation styles
#define DLG_SETUP_ROTSTYLE      4100


// demo type
#define IDC_DEMO_STRING         5000
#define IDC_DEMO_CLOCK          5001
#define IDC_TO_DEMOTYPE(n)      ( (n) - IDC_DEMO_STRING )

// sliders
#define IDC_STATIC_TESS         2051    // box around slider
#define IDC_STATIC_TESS_MIN     2052    // min label
#define IDC_STATIC_TESS_MAX     2053    // max label
#define IDC_STATIC_SIZE         2054
#define IDC_STATIC_SIZE_MIN     2055
#define IDC_STATIC_SIZE_MAX     2056

// rotation sliders
#define IDC_STATIC_ROTATION_GRP 5000

// configure text dialog box stuff
#define IDS_TEXT_TITLE          3001
#define DLG_TEXT_ENTER          3020
#define DLG_TEXT_SHOW           3021

// Choose font template
#define DLG_CF_TEMPLATE         6000
#define IDD_FONT                6001

// Default texture resource
#define IDB_DEFTEX              7000

#define SHELP_CONTENTS          01
#define SHELP_SHAPES            02
#define SHELP_PASSWORD          03
#define SHELP_COLOR             04
#define SHELP_MISC              05
#define SHELP_OVERVIEW          06

#endif // __sstext3d_h__
