/******************************Module*Header*******************************\
* Module Name: dialog.h
*
* Externals from dialog.c
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __dialog_h__
#define __dialog_h__

#include "dlgs.h"
#include "sscommon.h"

#ifdef __cplusplus
extern "C" {
#endif

extern float fTesselFact;
extern ULONG ulJointType;
extern ULONG ulSurfStyle;
extern ULONG ulTexQuality;
extern TEXFILE gTexFile[];
extern int gnTextures;

extern BOOL  bFlexMode;
extern BOOL  bMultiPipes;

extern void getIniSettings(void);


// surface styles
enum {
    SURFSTYLE_SOLID = 0,
    SURFSTYLE_TEX,
    SURFSTYLE_WIREFRAME
};

// Resource constants

#define IDS_SAVERNAME           1002
#define IDS_JOINTTYPE           1008
#define IDS_SURFSTYLE           1009
#define IDS_TEXQUAL             1010

#define IDS_FLEX                1020
#define IDS_MULTIPIPES          1021

//mf: should get rid of OFFSET...
#define IDS_TEXTURE_COUNT       1029
#define IDS_TEXTURE0            1030
#define IDS_TEXTURE1            1031
#define IDS_TEXTURE2            1032
#define IDS_TEXTURE3            1033
#define IDS_TEXTURE4            1034
#define IDS_TEXTURE5            1035
#define IDS_TEXTURE6            1036
#define IDS_TEXTURE7            1037
#define IDS_TEXOFFSET0          1050
#define IDS_TEXOFFSET1          1051
#define IDS_TEXOFFSET2          1052
#define IDS_TEXOFFSET3          1053
#define IDS_TEXOFFSET4          1054
#define IDS_TEXOFFSET5          1055
#define IDS_TEXOFFSET6          1056
#define IDS_TEXOFFSET7          1057

#define IDS_JOINT_ELBOW         1100
#define IDS_JOINT_BALL          1101
#define IDS_JOINT_MIXED         1102
#define IDS_JOINT_CYCLE         1103

#define DLG_SETUP_TESSEL        2001
#define DLG_SETUP_TEXTURE       2002
#define IDC_STATIC_TESS         2003
#define IDC_STATIC_TESS_MIN     2004
#define IDC_STATIC_TESS_MAX     2005
#define IDC_STATIC_TESS_GRP     2006
#define IDC_STATIC_TEXQUAL_GRP  2007

#define IDC_RADIO_SINGLE_PIPE       3000
#define IDC_RADIO_MULTIPLE_PIPES    3001

#define IDC_RADIO_NORMAL        3100
#define IDC_RADIO_FLEX          3101

#define IDC_STATIC_JOINTTYPE       2106

#define DLG_COMBO_JOINTTYPE     2200

enum {
    JOINT_ELBOW=0,
    JOINT_BALL,
    JOINT_MIXED,
    JOINT_CYCLE,
    NUM_JOINTTYPES
};

// In order for the IDC_TO_SURFSTYLE conversion macro to work, the radio buttons
// for surface styles must be kept contiguous.

#define IDC_RADIO_SOLID         2111
#define IDC_RADIO_TEX           2112
#define IDC_RADIO_WIREFRAME     2113
#define IDC_TO_SURFSTYLE(n)          ( (n) - IDC_RADIO_SOLID )


// In order for the IDC_TO_TEXQUAL conversion macro to work, the radio buttons
// for texture quality must be kept contiguous.

#define IDC_RADIO_TEXQUAL_DEFAULT   2121
#define IDC_RADIO_TEXQUAL_HIGH  2122
#define IDC_TO_TEXQUAL(n)       ( (n) - IDC_RADIO_TEXQUAL_DEFAULT )

#define TEXQUAL_DEFAULT         0
#define TEXQUAL_HIGH            1


// texture resources
#define IDB_DEFTEX              99
#define IDB_DEFTEX2            100 

// multi texture dialog

#ifdef __cplusplus
}
#endif

#endif // __dialog_h__
