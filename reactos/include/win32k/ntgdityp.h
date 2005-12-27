/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntgdityp.h
 * PURPOSE:         Win32 Shared GDI Types for NtGdi*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDITYP_
#define _NTGDITYP_

/* ENUMERATIONS **************************************************************/

typedef enum _ARCTYPE
{
    GdiTypeArc,
    GdiTypeArcTo,
    GdiTypeChord,
    GdiTypePie,
} ARCTYPE, *PARCTYPE;

typedef enum _PALFUNCTYPE
{
    GdiPalAnimate,
    GdiPalSetEntries,
    GdiPalGetEntries,
    GdiPalGetSystemEntries,
    GdiPalSetColorTable,
    GdiPalGetColorTable,
} PALFUNCTYPE, *PPALFUNCTYPE;

typedef enum _POLYFUNCTYPE
{
    GdiPolyPolygon = 1,
    GdiPolyPolyLine,
    GdiPolyBezier,
    GdiPolyLineTo,
    GdiPolyBezierTo,
    GdiPolyPolyRgn,
} POLYFUNCTYPE, *PPOLYFUNCTYPE;

typedef enum _GETDCDWORD
{
    GdiGetRelAbs = 1,
    GdiGetArcDirection = 4,
} GETDCDWORD, *PGETDCDWORD;

typedef enum _GETSETDCDWORD
{
    GdiGetSetArcDirection = 9,
} GETSETDCDWORD, *PGETSETDCDWORD;

typedef enum _GETDCPOINT
{
    GdiGetViewPortExt = 1,
    GdiGetDCOrg = 7,
} GETDCPOINT, *PGETDCPOINT;

typedef enum _TRANSFORMTYPE
{
    GdiDpToLp,
    GdiLpToDp,
} TRANSFORMTYPE, *PTRANSFORMTYPE;

#define GdiWorldSpaceToPageSpace    0x203

/* FIXME: Unknown */
typedef DWORD FULLSCREENCONTROL;
typedef DWORD LFTYPE;

/* TYPES *********************************************************************/

typedef PVOID KERNEL_PVOID;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;
typedef D3DNTHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEI;

/* FIXME: Unknown; easy to guess, usually based on public types and converted */
typedef struct _WIDTHDATA WIDTHDATA, *PWIDTHDATA;
typedef struct _TMDIFF TMDIFF, *PTMDIFF;
typedef struct _TMW_INTERNAL TMW_INTERNAL, *PTMW_INTERNAL;
typedef struct _ENUMFONTDATAW ENUMFONTDATAW, *PENUMFONTDATAW;
typedef struct _DEVCAPS DEVCAPS, *PDEVCAPS;
typedef struct _REALIZATION_INFO REALIZATION_INFO, *PREALIZATION_INFO;

#endif
