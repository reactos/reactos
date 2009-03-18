/*
 * Copyright (C) 1991-1998 by LCS/Telegraphics
 * Copyright (C) 2002 Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINTAB_H
#define __WINE_WINTAB_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/***********************************************************************
 * Messages 
 */
#ifndef NOWTMESSAGES

#define WT_DEFBASE    0x7FF0
#define WT_MAXOFFSET  0xF

#define _WT_PACKET(b)      ((b)+0)
#define _WT_CTXOPEN(b)     ((b)+1)
#define _WT_CTXCLOSE(b)    ((b)+2)
#define _WT_CTXUPDATE(b)   ((b)+3)
#define _WT_CTXOVERLAP(b)  ((b)+4)
#define _WT_PROXIMITY(b)   ((b)+5)
#define _WT_INFOCHANGE(b)  ((b)+6)
#define _WT_CSRCHANGE(b)   ((b)+7)  /* 1.1 */
#define _WT_MAX(b)         ((b)+WT_MAXOFFSET)

#define WT_PACKET      _WT_PACKET(WT_DEFBASE)
#define WT_CTXOPEN     _WT_CTXOPEN(WT_DEFBASE)
#define WT_CTXCLOSE    _WT_CTXCLOSE(WT_DEFBASE)
#define WT_CTXUPDATE   _WT_CTXUPDATE(WT_DEFBASE)
#define WT_CTXOVERLAP  _WT_CTXOVERLAP(WT_DEFBASE)
#define WT_PROXIMITY   _WT_PROXIMITY(WT_DEFBASE)
#define WT_INFOCHANGE  _WT_INFOCHANGE(WT_DEFBASE)
#define WT_CSRCHANGE   _WT_CSRCHANGE(WT_DEFBASE)  /* 1.1 */
#define WT_MAX         _WT_MAX(WT_DEFBASE)

#endif

/***********************************************************************
 * COMMON DATA DEFS
 */

DECLARE_HANDLE(HMGR);     /* manager handle */
DECLARE_HANDLE(HCTX);     /* context handle */
DECLARE_HANDLE(HWTHOOK);  /* hook handle */

typedef DWORD WTPKT;   /* packet mask */

#ifndef NOWTPKT

/* WTPKT bits */
#define PK_CONTEXT           0x0001  /* reporting context */
#define PK_STATUS            0x0002  /* status bits */
#define PK_TIME              0x0004  /* time stamp */
#define PK_CHANGED           0x0008  /* change bit vector */
#define PK_SERIAL_NUMBER     0x0010  /* packet serial number */
#define PK_CURSOR            0x0020  /* reporting cursor */
#define PK_BUTTONS           0x0040  /* button information */
#define PK_X                 0x0080  /* x axis */
#define PK_Y                 0x0100  /* y axis */
#define PK_Z                 0x0200  /* z axis */
#define PK_NORMAL_PRESSURE   0x0400  /* normal or tip pressure */
#define PK_TANGENT_PRESSURE  0x0800  /* tangential or barrel pressure */
#define PK_ORIENTATION       0x1000  /* orientation info: tilts */
#define PK_ROTATION          0x2000  /* rotation info; 1.1 */

#endif

typedef DWORD FIX32; /* fixed-point arithmetic type */

#ifndef NOFIX32

#define INT(x) HIWORD(x)
#define FRAC(x) LOWORD(x)

#define CASTFIX32(x) ((FIX32)((x)*65536L))

#define ROUND(x) (INT(x) + (FRAC(x) > (WORD)0x8000))

#define FIX_MUL(c, a, b) \
    (c = (((DWORD)FRAC(a) * FRAC(b)) >> 16) + \
           (DWORD)INT(a) * FRAC(b) + \
           (DWORD)INT(b) * FRAC(a) + \
          ((DWORD)INT(a) * INT(b) << 16))

#ifdef _WINDLL
# define FIX_DIV_SC static
#else
# define FIX_DIV_SC
# endif

#define FIX_DIV(c, a, b) \
    { \
        FIX_DIV_SC DWORD temp, rem, btemp; \
        \
        /* fraction done bytewise */ \
        temp = ((a / b) << 16); \
        rem = a % b;   \
        btemp = b; \
        if (INT(btemp) < 256) { \
            rem <<= 8; \
        } else { \
            btemp >>= 8; \
        } \
        temp += ((rem / btemp) << 8); \
        rem %= btemp; \
        rem <<= 8; \
        temp += rem / btemp; \
        c = temp; \
    }

#endif

/***********************************************************************
 * INFO DATA DEFS
 */

#ifndef NOWTINFO

#ifndef NOWTAXIS

typedef struct tagAXIS {
    LONG   axMin;
    LONG   axMax;
    UINT   axUnits;
    FIX32  axResolution;
} AXIS, *PAXIS, *NPAXIS, *LPAXIS;

/* unit specifiers */
#define TU_NONE         0
#define TU_INCHES       1
#define TU_CENTIMETERS  2
#define TU_CIRCLE       3

#endif

#ifndef NOWTSYSBUTTONS

/* system button assignment values */
#define SBN_NONE       0x00
#define SBN_LCLICK     0x01
#define SBN_LDBLCLICK  0x02
#define SBN_LDRAG      0x03
#define SBN_RCLICK     0x04
#define SBN_RDBLCLICK  0x05
#define SBN_RDRAG      0x06
#define SBN_MCLICK     0x07
#define SBN_MDBLCLICK  0x08
#define SBN_MDRAG      0x09

/* for Pen Windows */
#define SBN_PTCLICK     0x10
#define SBN_PTDBLCLICK  0x20
#define SBN_PTDRAG      0x30
#define SBN_PNCLICK     0x40
#define SBN_PNDBLCLICK  0x50
#define SBN_PNDRAG      0x60
#define SBN_P1CLICK     0x70
#define SBN_P1DBLCLICK  0x80
#define SBN_P1DRAG      0x90
#define SBN_P2CLICK     0xA0
#define SBN_P2DBLCLICK  0xB0
#define SBN_P2DRAG      0xC0
#define SBN_P3CLICK     0xD0
#define SBN_P3DBLCLICK  0xE0
#define SBN_P3DRAG      0xF0

#endif

#ifndef NOWTCAPABILITIES

/* hardware capabilities */
#define HWC_INTEGRATED      0x0001
#define HWC_TOUCH           0x0002
#define HWC_HARDPROX        0x0004
#define HWC_PHYSID_CURSORS  0x0008  /* 1.1 */
#endif

#ifndef NOWTIFC

#ifndef NOWTCURSORS 

/* cursor capabilities */
#define CRC_MULTIMODE  0x0001  /* 1.1 */
#define CRC_AGGREGATE  0x0002  /* 1.1 */
#define CRC_INVERT     0x0004  /* 1.1 */

#endif 

/* info categories */
#define WTI_INTERFACE    1
#define IFC_WINTABID     1
#define IFC_SPECVERSION  2
#define IFC_IMPLVERSION  3
#define IFC_NDEVICES     4
#define IFC_NCURSORS     5
#define IFC_NCONTEXTS    6
#define IFC_CTXOPTIONS   7
#define IFC_CTXSAVESIZE  8
#define IFC_NEXTENSIONS  9
#define IFC_NMANAGERS    10
#define IFC_MAX          10

#endif

#ifndef NOWTSTATUS

#define WTI_STATUS     2
#define STA_CONTEXTS   1
#define STA_SYSCTXS    2
#define STA_PKTRATE    3
#define STA_PKTDATA    4
#define STA_MANAGERS   5
#define STA_SYSTEM     6
#define STA_BUTTONUSE  7
#define STA_SYSBTNUSE  8
#define STA_MAX        8

#endif

#ifndef NOWTDEFCONTEXT

#define WTI_DEFCONTEXT  3
#define WTI_DEFSYSCTX   4
#define WTI_DDCTXS      400 /* 1.1 */
#define WTI_DSCTXS      500 /* 1.1 */
#define CTX_NAME        1
#define CTX_OPTIONS     2
#define CTX_STATUS      3
#define CTX_LOCKS       4
#define CTX_MSGBASE     5
#define CTX_DEVICE      6
#define CTX_PKTRATE     7
#define CTX_PKTDATA     8
#define CTX_PKTMODE     9
#define CTX_MOVEMASK    10
#define CTX_BTNDNMASK   11
#define CTX_BTNUPMASK   12
#define CTX_INORGX      13
#define CTX_INORGY      14
#define CTX_INORGZ      15
#define CTX_INEXTX      16
#define CTX_INEXTY      17
#define CTX_INEXTZ      18
#define CTX_OUTORGX     19
#define CTX_OUTORGY     20
#define CTX_OUTORGZ     21
#define CTX_OUTEXTX     22
#define CTX_OUTEXTY     23
#define CTX_OUTEXTZ     24
#define CTX_SENSX       25
#define CTX_SENSY       26
#define CTX_SENSZ       27
#define CTX_SYSMODE     28
#define CTX_SYSORGX     29
#define CTX_SYSORGY     30
#define CTX_SYSEXTX     31
#define CTX_SYSEXTY     32
#define CTX_SYSSENSX    33
#define CTX_SYSSENSY    34
#define CTX_MAX         34

#endif

#ifndef NOWTDEVICES

#define WTI_DEVICES      100
#define DVC_NAME         1
#define DVC_HARDWARE     2
#define DVC_NCSRTYPES    3
#define DVC_FIRSTCSR     4
#define DVC_PKTRATE      5
#define DVC_PKTDATA      6
#define DVC_PKTMODE      7
#define DVC_CSRDATA      8
#define DVC_XMARGIN      9
#define DVC_YMARGIN      10
#define DVC_ZMARGIN      11
#define DVC_X            12
#define DVC_Y            13
#define DVC_Z            14
#define DVC_NPRESSURE    15
#define DVC_TPRESSURE    16
#define DVC_ORIENTATION  17
#define DVC_ROTATION     18  /* 1.1 */
#define DVC_PNPID        19  /* 1.1 */
#define DVC_MAX          19

#endif

#ifndef NOWTCURSORS

#define WTI_CURSORS      200
#define CSR_NAME         1
#define CSR_ACTIVE       2
#define CSR_PKTDATA      3
#define CSR_BUTTONS      4
#define CSR_BUTTONBITS   5
#define CSR_BTNNAMES     6
#define CSR_BUTTONMAP    7
#define CSR_SYSBTNMAP    8
#define CSR_NPBUTTON     9
#define CSR_NPBTNMARKS   10
#define CSR_NPRESPONSE   11
#define CSR_TPBUTTON     12
#define CSR_TPBTNMARKS   13
#define CSR_TPRESPONSE   14
#define CSR_PHYSID       15  /* 1.1 */
#define CSR_MODE         16  /* 1.1 */
#define CSR_MINPKTDATA   17  /* 1.1 */
#define CSR_MINBUTTONS   18  /* 1.1 */
#define CSR_CAPABILITIES 19  /* 1.1 */
/* from http://www.wacomeng.com/devsupport/ibmpc/wacomwindevfaq.html */
#define CSR_TYPE        20
#define CSR_MAX         20

#endif

#ifndef NOWTEXTENSIONS

#define WTI_EXTENSIONS  300
#define EXT_NAME        1
#define EXT_TAG         2
#define EXT_MASK        3
#define EXT_SIZE        4
#define EXT_AXES        5
#define EXT_DEFAULT     6
#define EXT_DEFCONTEXT  7
#define EXT_DEFSYSCTX   8
#define EXT_CURSORS     9 
#define EXT_MAX         109  /* Allow 100 cursors */

#endif

#endif

/***********************************************************************
 * CONTEXT DATA DEFS
 */

#define LCNAMELEN 40
#define LC_NAMELEN 40

typedef struct tagLOGCONTEXTA {
    char   lcName[LCNAMELEN];
    UINT   lcOptions;
    UINT   lcStatus;
    UINT   lcLocks;
    UINT   lcMsgBase;
    UINT   lcDevice;
    UINT   lcPktRate;
    WTPKT  lcPktData;
    WTPKT  lcPktMode;
    WTPKT  lcMoveMask;
    DWORD  lcBtnDnMask;
    DWORD  lcBtnUpMask;
    LONG   lcInOrgX;
    LONG   lcInOrgY;
    LONG   lcInOrgZ;
    LONG   lcInExtX;
    LONG   lcInExtY;
    LONG   lcInExtZ;
    LONG   lcOutOrgX;
    LONG   lcOutOrgY;
    LONG   lcOutOrgZ;
    LONG   lcOutExtX;
    LONG   lcOutExtY;
    LONG   lcOutExtZ;
    FIX32  lcSensX;
    FIX32  lcSensY;
    FIX32  lcSensZ;
    BOOL   lcSysMode;
    int    lcSysOrgX;
    int    lcSysOrgY;
    int    lcSysExtX;
    int    lcSysExtY;
    FIX32  lcSysSensX;
    FIX32  lcSysSensY;
} LOGCONTEXTA, *PLOGCONTEXTA, *NPLOGCONTEXTA, *LPLOGCONTEXTA;

typedef struct tagLOGCONTEXTW {
    WCHAR  lcName[LCNAMELEN];
    UINT   lcOptions;
    UINT   lcStatus;
    UINT   lcLocks;
    UINT   lcMsgBase;
    UINT   lcDevice;
    UINT   lcPktRate;
    WTPKT  lcPktData;
    WTPKT  lcPktMode;
    WTPKT  lcMoveMask;
    DWORD  lcBtnDnMask;
    DWORD  lcBtnUpMask;
    LONG   lcInOrgX;
    LONG   lcInOrgY;
    LONG   lcInOrgZ;
    LONG   lcInExtX;
    LONG   lcInExtY;
    LONG   lcInExtZ;
    LONG   lcOutOrgX;
    LONG   lcOutOrgY;
    LONG   lcOutOrgZ;
    LONG   lcOutExtX;
    LONG   lcOutExtY;
    LONG   lcOutExtZ;
    FIX32  lcSensX;
    FIX32  lcSensY;
    FIX32  lcSensZ;
    BOOL   lcSysMode;
    int    lcSysOrgX;
    int    lcSysOrgY;
    int    lcSysExtX;
    int    lcSysExtY;
    FIX32  lcSysSensX;
    FIX32  lcSysSensY;
} LOGCONTEXTW, *PLOGCONTEXTW, *NPLOGCONTEXTW, *LPLOGCONTEXTW;

DECL_WINELIB_TYPE_AW(LOGCONTEXT)
DECL_WINELIB_TYPE_AW(PLOGCONTEXT)
DECL_WINELIB_TYPE_AW(NPLOGCONTEXT)
DECL_WINELIB_TYPE_AW(LPLOGCONTEXT)

/* context option values */
#define CXO_SYSTEM       0x0001
#define CXO_PEN          0x0002
#define CXO_MESSAGES     0x0004
#define CXO_MARGIN       0x8000
#define CXO_MGNINSIDE    0x4000
#define CXO_CSRMESSAGES  0x0008  /* 1.1 */

/* context status values */
#define CXS_DISABLED  0x0001
#define CXS_OBSCURED  0x0002
#define CXS_ONTOP     0x0004

/* context lock values */
#define CXL_INSIZE       0x0001
#define CXL_INASPECT     0x0002
#define CXL_SENSITIVITY  0x0004
#define CXL_MARGIN       0x0008
#define CXL_SYSOUT       0x0010

/***********************************************************************
 * EVENT DATA DEFS
 */

/* For packet structure definition, see pktdef.h */

/* packet status values */
#define TPS_PROXIMITY  0x0001
#define TPS_QUEUE_ERR  0x0002
#define TPS_MARGIN     0x0004
#define TPS_GRAB       0x0008
#define TPS_INVERT     0x0010  /* 1.1 */

typedef struct tagORIENTATION {
    int orAzimuth;
    int orAltitude;
    int orTwist;
} ORIENTATION, *PORIENTATION, *NPORIENTATION, *LPORIENTATION;

typedef struct tagROTATION {  /* 1.1 */
    int roPitch;
    int roRoll;
    int roYaw;
} ROTATION, *PROTATION, *NPROTATION, *LPROTATION;

/* grandfather in obsolete member names. */
#define rotPitch  roPitch
#define rotRoll   roRoll
#define rotYaw    roYaw

/* relative buttons */
#define TBN_NONE  0
#define TBN_UP    1
#define TBN_DOWN  2

/***********************************************************************
 * DEVICE CONFIG CONSTANTS
 */

#ifndef NOWTDEVCFG

#define WTDC_NONE     0
#define WTDC_CANCEL   1
#define WTDC_OK       2
#define WTDC_RESTART  3

#endif

/***********************************************************************
 * HOOK CONSTANTS
 */

#ifndef NOWTHOOKS

#define WTH_PLAYBACK  1
#define WTH_RECORD    2

#define WTHC_GETLPLPFN   (-3)
#define WTHC_LPLPFNNEXT  (-2)
#define WTHC_LPFNNEXT    (-1)
#define WTHC_ACTION      0
#define WTHC_GETNEXT     1
#define WTHC_SKIP        2

#endif

/***********************************************************************
 * PREFERENCE FUNCTION CONSTANTS
 */

#ifndef NOWTPREF

#define WTP_LPDEFAULT  ((LPVOID)-1L)
#define WTP_DWDEFAULT  ((DWORD)-1L)

#endif

/***********************************************************************
 * EXTENSION TAGS AND CONSTANTS
 */

#ifndef NOWTEXTENSIONS

/* constants for use with pktdef.h */
#define PKEXT_ABSOLUTE    1
#define PKEXT_RELATIVE    2

/* Extension tags. */
#define WTX_OBT       0  /* Out of bounds tracking */
#define WTX_FKEYS     1  /* Function keys */
#define WTX_TILT      2  /* Raw Cartesian tilt; 1.1 */
#define WTX_CSRMASK   3  /* select input by cursor type; 1.1 */
#define WTX_XBTNMASK  4  /* Extended button mask; 1.1 */

typedef struct tagXBTNMASK {
    BYTE xBtnDnMask[32];
    BYTE xBtnUpMask[32];
} XBTNMASK;

typedef struct tagTILT {  /* 1.1 */
    int tiltX;
    int tiltY;
} TILT;

#endif

/***********************************************************************
 * Functions
 */

#ifndef NOWTCALLBACKS

#ifndef NOWTMANAGERFXNS

/* callback function types */
typedef BOOL (WINAPI * WTENUMPROC)(HCTX, LPARAM);  /* changed CALLBACK->WINAPI, 1.1 */
typedef BOOL (WINAPI * WTCONFIGPROC)(HCTX, HWND);
typedef LRESULT (WINAPI * WTHOOKPROC)(int, WPARAM, LPARAM);
typedef WTHOOKPROC *LPWTHOOKPROC;

#endif

#endif

#ifndef NOWTFUNCTIONS

#ifndef NOWTBASICFXNS
/* BASIC FUNCTIONS */

#define ORD_WTInfoA       20
#define ORD_WTInfoW       1020
#define ORD_WTInfo        WINELIB_NAME_AW(ORD_WTInfo)
#define ORD_WTOpenA       21
#define ORD_WTOpenW       1021
#define ORD_WTOpen        WINELIB_NAME_AW(ORD_WTOpen)
#define ORD_WTClose       22
#define ORD_WTPacketsGet  23
#define ORD_WTPacket      24

UINT WINAPI WTInfoA(UINT, UINT, LPVOID);
UINT WINAPI WTInfoW(UINT, UINT, LPVOID);
#define WTInfo WINELIB_NAME_AW(WTInfo)
HCTX WINAPI WTOpenA(HWND, LPLOGCONTEXTA, BOOL);
HCTX WINAPI WTOpenW(HWND, LPLOGCONTEXTW, BOOL);
#define WTOpen WINELIB_NAME_AW(WTOpen)
BOOL WINAPI WTClose(HCTX);
int  WINAPI WTPacketsGet(HCTX, int, LPVOID);
BOOL WINAPI WTPacket(HCTX, UINT, LPVOID);

#endif

#ifndef NOWTVISIBILITYFXNS
/* VISIBILITY FUNCTIONS */

#define ORD_WTEnable   40
#define ORD_WTOverlap  41

BOOL WINAPI WTEnable(HCTX, BOOL);
BOOL WINAPI WTOverlap(HCTX, BOOL);

#endif

#ifndef NOWTCTXEDITFXNS
/* CONTEXT EDITING FUNCTIONS */

#define ORD_WTConfig   60
#define ORD_WTGetA     61
#define ORD_WTGetW     1061
#define ORD_WTGet      WINELIB_NAME_AW(ORD_WTGet)
#define ORD_WTSetA     62
#define ORD_WTSetW     1062
#define ORD_WTSet      WINELIB_NAME_AW(ORD_WTSet)
#define ORD_WTExtGet   63
#define ORD_WTExtSet   64
#define ORD_WTSave     65
#define ORD_WTRestore  66

BOOL WINAPI WTConfig(HCTX, HWND);
BOOL WINAPI WTGetA(HCTX, LPLOGCONTEXTA);
BOOL WINAPI WTGetW(HCTX, LPLOGCONTEXTW);
#define WTGet WINELIB_NAME_AW(WTGet)
BOOL WINAPI WTSetA(HCTX, LPLOGCONTEXTA);
BOOL WINAPI WTSetW(HCTX, LPLOGCONTEXTW);
#define WTSet WINELIB_NAME_AW(WTSet)
BOOL WINAPI WTExtGet(HCTX, UINT, LPVOID);
BOOL WINAPI WTExtSet(HCTX, UINT, LPVOID);
BOOL WINAPI WTSave(HCTX, LPVOID);
HCTX WINAPI WTRestore(HWND, LPVOID, BOOL);

#endif

#ifndef NOWTQUEUEFXNS
/* ADVANCED PACKET AND QUEUE FUNCTIONS */

#define ORD_WTPacketsPeek   80
#define ORD_WTDataGet       81
#define ORD_WTDataPeek      82
#define ORD_WTQueueSizeGet  84
#define ORD_WTQueueSizeSet  85

int WINAPI WTPacketsPeek(HCTX, int, LPVOID);
int WINAPI WTDataGet(HCTX, UINT, UINT, int, LPVOID, LPINT);
int WINAPI WTDataPeek(HCTX, UINT, UINT, int, LPVOID, LPINT);
int WINAPI WTQueueSizeGet(HCTX);
BOOL WINAPI WTQueueSizeSet(HCTX, int);

#endif

#ifndef NOWTHMGRFXNS
/* MANAGER HANDLE FUNCTIONS */

#define ORD_WTMgrOpen   100
#define ORD_WTMgrClose  101

HMGR WINAPI WTMgrOpen(HWND, UINT);
BOOL WINAPI WTMgrClose(HMGR);

#endif

#ifndef NOWTMGRCTXFXNS
/* MANAGER CONTEXT FUNCTIONS */

#define ORD_WTMgrContextEnum   120
#define ORD_WTMgrContextOwner  121
#define ORD_WTMgrDefContext    122
#define ORD_WTMgrDefContextEx  206

BOOL WINAPI WTMgrContextEnum(HMGR, WTENUMPROC, LPARAM);
HWND WINAPI WTMgrContextOwner(HMGR, HCTX);
HCTX WINAPI WTMgrDefContext(HMGR, BOOL);
HCTX WINAPI WTMgrDefContextEx(HMGR, UINT, BOOL);  /* 1.1 */

#endif

#ifndef NOWTMGRCONFIGFXNS
/* MANAGER CONFIG BOX  FUNCTIONS */

#define ORD_WTMgrDeviceConfig 140

UINT WINAPI WTMgrDeviceConfig(HMGR, UINT, HWND);

#endif

#ifndef NOWTMGRHOOKFXNS
/* MANAGER PACKET HOOK FUNCTIONS */

/* OBSOLETE IN WIN32! */

#endif

#ifndef NOWTMGRPREFFXNS
/* MANAGER PREFERENCE DATA FUNCTIONS */

#define ORD_WTMgrExt                  180
#define ORD_WTMgrCsrEnable            181
#define ORD_WTMgrCsrButtonMap         182
#define ORD_WTMgrCsrPressureBtnMarks  183
#define ORD_WTMgrCsrPressureResponse  184
#define ORD_WTMgrCsrExt               185

BOOL WINAPI WTMgrExt(HMGR, UINT, LPVOID);
BOOL WINAPI WTMgrCsrEnable(HMGR, UINT, BOOL);
BOOL WINAPI WTMgrCsrButtonMap(HMGR, UINT, LPBYTE, LPBYTE);
BOOL WINAPI WTMgrCsrPressureBtnMarks(HMGR, UINT, DWORD, DWORD);
BOOL WINAPI WTMgrCsrPressureResponse(HMGR, UINT, UINT *, UINT *);
BOOL WINAPI WTMgrCsrExt(HMGR, UINT, UINT, LPVOID);

#endif

/***********************************************************************
 * Win32 replacements for non-portable functions.
 */

#ifndef NOWTQUEUEFXNS
/* ADVANCED PACKET AND QUEUE FUNCTIONS */

#define ORD_WTQueuePacketsEx  200

BOOL WINAPI WTQueuePacketsEx(HCTX, UINT *, UINT *);

#endif

#ifndef NOWTMGRCONFIGFXNS
/* MANAGER CONFIG BOX FUNCTIONS */

#define ORD_WTMgrConfigReplaceExA  202
#define ORD_WTMgrConfigReplaceExW  1202
#define ORD_WTMgrConfigReplaceEx   WINELIB_NAME_AW(ORD_WTMgrConfigReplaceEx)

BOOL WINAPI WTMgrConfigReplaceExA(HMGR, BOOL, LPSTR, LPSTR);
BOOL WINAPI WTMgrConfigReplaceExW(HMGR, BOOL, LPWSTR, LPSTR);
#define WTMgrConfigReplaceEx WINELIB_NAME_AW(WTMgrConfigReplaceEx)

#endif

#ifndef NOWTMGRHOOKFXNS
/* MANAGER PACKET HOOK FUNCTIONS */

#define ORD_WTMgrPacketHookExA   203
#define ORD_WTMgrPacketHookExW   1203
#define ORD_WTMgrPacketHookEx    WINELIB_NAME_AW(ORD_WTMgrPacketHookEx)
#define ORD_WTMgrPacketUnhook    204
#define ORD_WTMgrPacketHookNext  205

HWTHOOK WINAPI WTMgrPacketHookExA(HMGR, int, LPSTR, LPSTR);
HWTHOOK WINAPI WTMgrPacketHookExW(HMGR, int, LPWSTR, LPSTR);
#define WTMgrPacketHookEx WINELIB_NAME_AW(WTMgrPacketHookEx)
BOOL    WINAPI WTMgrPacketUnhook(HWTHOOK);
LRESULT WINAPI WTMgrPacketHookNext(HWTHOOK, int, WPARAM, LPARAM);

#endif

#ifndef NOWTMGRPREFFXNS
/* MANAGER PREFERENCE DATA FUNCTIONS */

#define ORD_WTMgrCsrPressureBtnMarksEx  201

BOOL WINAPI WTMgrCsrPressureBtnMarksEx(HMGR, UINT, UINT *, UINT *);

#endif

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* defined(__WINE_WINTAB_H */
