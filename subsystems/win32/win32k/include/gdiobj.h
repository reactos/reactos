/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/include/gdiobj.h
 * PURPOSE:         GDI Structures
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */
 
#ifndef _WIN32K_GDIOBJ_H
#define _WIN32K_GDIOBJ_H

typedef struct _BASEOBJECT
{
    HGDIOBJ hHmgr;
    ULONG ulShareCount;
    USHORT cExclusiveLock;
    USHORT BaseFlags;
    struct _PW32THREAD *Tid;
} BASEOBJECT, *POBJ;

typedef struct _GDIBRUSHOBJ
{
    BASEOBJECT BaseObject;
    ULONG ulStyle;
    HBITMAP hbmPattern;
    HANDLE hbmClient;
    FLONG flAttrs;
    ULONG ulBrushUnique;
    BRUSH_ATTR *pBrushAttr;
    BRUSH_ATTR BrushAttr;
    POINT ptOrigin;
    ULONG bCacheGrabbed;
    COLORREF crBack;
    COLORREF crFore;
    ULONG ulPalTime;
    ULONG ulSurfTime;
    PVOID ulRealization;
    ULONG Unknown4C[3];
    POINT ptPenWidth;
    ULONG ulPenStyle;
    DWORD *pStyle;
    DWORD dwStyleCount;
    ULONG Unknown6C;
} GDIBRUSHOBJ, *PGDIBRUSHOBJ;

// EXtended CLip and Window Region Object
// Multipurpose E/X/CLIPOBJ and E/WNDOBJ structure
typedef struct _XCLIPOBJ
{
    WNDOBJ exClipWnd;
    PVOID pClipRgn;
    RECTL rclClipRgn;
    PVOID pscanClipRgn;
    DWORD cScans;
    DWORD Unknown1;
    ULONG ulBSize;
    LONG lscnSize;
    ULONG ulObjSize;
    ULONG iDirection;
    ULONG ClipType;
    DWORD Unknown2;
    LONG lUpDown;
    DWORD Unknown3;
    BOOL ShouldDoAll;
    DWORD nComplexity;
    PVOID pUnknown;
} XCLIPOBJ, *PXCLIPOBJ;

typedef struct _SCAN
{
    LONG scnPntCnt;
    LONG scnPntTop;
    LONG scnPntBottom;
    LONG scnPntX[2];
} SCAN, *PSCAN;

typedef struct _SCAN1
{
    LONG scnPntCnt;
    struct _SCAN2 *scnPnt[3];
    LONG scnPntCntToo;
} SCAN1, *PSCAN1;

typedef struct _SCAN2
{
    LONG scnPntTop;
    LONG scnPntBottom;
    LONG scnPntX[2];
} SCAN2, *PSCAN2;

typedef struct _REGION
{
   BASEOBJECT BaseObject;
   unsigned   sizeObj;
   ULONG      iUniq;      // from Clip object
   DWORD      nRefCount;   // inc/dec Ref count if 0 deleteRGNOBJ
   PSCAN pscnTail;
   unsigned   sizeRgn;
   unsigned   cScans;
   RECTL rcl;
   SCAN scnHead1[1];
   SCAN1 scnHead2[1];
} REGION, *PREGION;

#endif // _WIN32K_GDIOBJ_H
