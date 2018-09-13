/*****************************************************************************\
*                                                                             *
* custcntl.h -  Custom Control Library header file                            *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_CUSTCNTL
#define _INC_CUSTCNTL

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/*
 * Every custom control DLL must have three functions present,
 * and they must be exported by the following ordinals.
 */
#define CCINFOORD       2       /* information function ordinal */
#define CCSTYLEORD      3       /* styles function ordinal */
#define CCFLAGSORD      4       /* translate flags function ordinal */

/* general size definitions */
#define CTLTYPES        12      /* max number of control types */
#define CTLDESCR        22      /* max size of description */
#define CTLCLASS        20      /* max size of class name */
#define CTLTITLE        94      /* max size of control text */

/*
 * CONTROL STYLE DATA STRUCTURE
 *
 * This data structure is used by the class style dialog function
 * to set and/or reset various control attributes.
 *
 */
typedef struct tagCTLSTYLE
{
    UINT    wX;                 /* x origin of control */
    UINT    wY;                 /* y origin of control */
    UINT    wCx;                /* width of control */
    UINT    wCy;                /* height of control */
    UINT    wId;                /* control child id */
    DWORD   dwStyle;            /* control style */
    char    szClass[CTLCLASS];  /* name of control class */
    char    szTitle[CTLTITLE];  /* control text */
} CTLSTYLE;
typedef CTLSTYLE *      PCTLSTYLE;
typedef CTLSTYLE FAR*   LPCTLSTYLE;

/*
 * CONTROL DATA STRUCTURE
 *
 * This data structure is returned by the control options function
 * when inquiring about the capabilities of a particular control.
 * Each control may contain various types (with predefined style
 * bits) under one general class.
 *
 * The width and height fields are used to provide the host
 * application with a suggested size.  The values in these fields
 * are in rc coordinates.
 *
 */
typedef struct tagCTLTYPE
{
    UINT    wType;              /* type style */
    UINT    wWidth;             /* suggested width */
    UINT    wHeight;            /* suggested height */
    DWORD   dwStyle;            /* default style */
    char    szDescr[CTLDESCR];  /* description */
} CTLTYPE;

typedef struct tagCTLINFO
{
    UINT    wVersion;           /* control version */
    UINT    wCtlTypes;          /* control types */
    char    szClass[CTLCLASS];  /* control class name */
    char    szTitle[CTLTITLE];  /* control title */
    char    szReserved[10];     /* reserved for future use */
    CTLTYPE Type[CTLTYPES];     /* control type list */
} CTLINFO;
typedef CTLINFO *       PCTLINFO;
typedef CTLINFO FAR*    LPCTLINFO;

/* These two function prototypes are used by the dialog editor */
#ifdef STRICT
typedef DWORD   (CALLBACK* LPFNSTRTOID)(LPCSTR);
#else
typedef DWORD   (CALLBACK* LPFNSTRTOID)(LPSTR);
#endif
typedef UINT    (CALLBACK* LPFNIDTOSTR)(UINT, LPSTR, UINT);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* RC_INVOKED */

#endif  /* _INC_CUSTCNTL */
