/******************************Module*Header*******************************\
* Module Name: mazedlg.h
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __dialog_h__
#define __dialog_h__

#include "dlgs.h"
#include "sscommon.h"
#include "maze_std.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IDS_SCREENSAVERTITLE    1020
#define IDS_SAVERNAME           1002

// structure to pull in texture info from registry
typedef struct {
    BOOL    bTex;       // texture enable/disable
    BOOL    bDefTex;    // default texture enable/disable
    int     iDefTex;    // default texture index
    TEXFILE texFile;    // user texture file
} TEX_INFO;

// registry strings

#define IDS_TEXTURE_ENABLE              1100
#define IDS_DEFAULT_TEXTURE_ENABLE      1101

#define IDS_WALL_TEXTURE_FILE           1110
#define IDS_FLOOR_TEXTURE_FILE          1111
#define IDS_CEILING_TEXTURE_FILE        1112

#define IDS_WALL_TEXTURE_OFFSET         1120
#define IDS_FLOOR_TEXTURE_OFFSET        1121
#define IDS_CEILING_TEXTURE_OFFSET      1122

#define IDS_DEF_WALL_TEXTURE            1130
#define IDS_DEF_FLOOR_TEXTURE           1131
#define IDS_DEF_CEILING_TEXTURE         1132

#define IDS_OVERLAY                     1200

#define IDS_TURBOMODE                   1250

#define IDS_IMAGEQUAL                   1300

#define IDS_NRATS                       1400

// Dialog box defines

// Imagequal combobox strings
#define IDS_IMAGEQUAL_DEFAULT     1301
#define IDS_IMAGEQUAL_HIGH        1302

#define MIN_SLIDER              0
#define MAX_SLIDER              100

// Size slider
#define DLG_SLIDER_SIZE         2000
#define IDC_STATIC_SIZE         2001
#define IDC_STATIC_MIN          2002
#define IDC_STATIC_MAX          2003

// Maze overly check box
#define DLG_CHECK_OVERLAY       2008

// Turbo mode check box
#define DLG_CHECK_TURBOMODE     2009

// Buttons to choose textures
#define DLG_BUTTON_WALLS_TEX    3100
#define DLG_BUTTON_FLOOR_TEX    3101
#define DLG_BUTTON_CEILING_TEX  3102
#define DLG_BUTTON_TEX_TO_SURFACE(n)    ( (n) - DLG_BUTTON_WALLS_TEX )

// Previews for surfaces (now just texture, but could show like colors)
#define DLG_PREVIEW_WALLS        3200
#define DLG_PREVIEW_FLOOR        3201
#define DLG_PREVIEW_CEILING      3202
#define DLG_PREVIEW_TO_SURFACE(n)    ( (n) - DLG_PREVIEW_WALLS )
#define DLG_SURFACE_TO_PREVIEW(n)    ( (n) + DLG_PREVIEW_WALLS )

// Spin controls for texture previews
//mf: add TEX suffix
#define DLG_SPIN_WALLS           3300
#define DLG_SPIN_FLOOR           3301
#define DLG_SPIN_CEILING         3302
#define DLG_SPIN_TEX_TO_SURFACE(n)    ( (n) - DLG_SPIN_WALLS )

// Resource bitmap id's
#define IDB_BRICK                100
#define IDB_WOOD                 101
#define IDB_CASTLE               102
#define IDB_START                103
#define IDB_END                  104
#define IDB_RAT                  105
#define IDB_AD                   106
#define IDB_COVER                107

#define IDB_CURL4                120
#define IDB_BHOLE4               121
#define IDB_SNOWFLAK             125
#define IDB_SWIRLX4               127

// Image quality box
#define DLG_COMBO_IMAGEQUAL      3500
#define IDC_STATIC_IMAGEQUAL     3501

// Choose texture dialog box

#define DLG_TEXTURE_CONFIGURE   4000

#define IDC_RADIO_TEX_DEFAULT   4010
#define IDC_RADIO_TEX_CHOOSE    4011
#define DLG_BUTTON_TEX_CHOOSE   4012

extern void getIniSettings();
extern TEX_INFO gTexInfo[];
extern int      giSize; // window size
extern BOOL     gbTurboMode;  // turbo mode enable/disable
extern int      giImageQual;  // Image quality (dithering)

#ifdef __cplusplus
}
#endif

#endif // __dialog_h__
