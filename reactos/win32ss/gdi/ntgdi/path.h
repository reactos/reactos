#pragma once

/* DCPATH flPath */
enum _DCPATHFLAGS
{
    DCPATH_ACTIVE    = 0x0001,
    DCPATH_SAVE      = 0x0002,
    DCPATH_CLOCKWISE = 0x0004,

    /* ReactOS only */
    DCPATH_SAVESTATE = 0x80000000
};

typedef HGDIOBJ HPATH, *PHPATH;

typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct _PATH
{
  BASEOBJECT   BaseObject;

  RECTFX       rcfxBoundBox;
  POINTFX      ptfxSubPathStart;

  // Things to convert from:
  DWORD        state;
  POINT        *pPoints;
  BYTE         *pFlags;
  int          numEntriesUsed;
  int          numEntriesAllocated;
  BOOL         newStroke;
} PATH, *PPATH;

typedef struct _EPATHOBJ
{
  PATHOBJ po;
  PPATH   pPath;
} EPATHOBJ, *PEPATHOBJ;

#define  PATH_AllocPath() ((PPATH) GDIOBJ_AllocObj(GDIObjType_PATH_TYPE))
#define  PATH_AllocPathWithHandle() ((PPATH) GDIOBJ_AllocObjWithHandle (GDI_OBJECT_TYPE_PATH, sizeof(PATH)))
#define  PATH_FreePath(pPath)  GDIOBJ_FreeObj((POBJ)pPath, GDIObjType_PATH_TYPE)
#define  PATH_FreeExtPathByHandle(hPath) GDIOBJ_FreeObjByHandle((HGDIOBJ) hPath, GDI_OBJECT_TYPE_PATH)
#define  PATH_LockPath(hPath) ((PPATH)GDIOBJ_ShareLockObj((HGDIOBJ)hPath, GDI_OBJECT_TYPE_PATH))
#define  PATH_UnlockPath(pPath) GDIOBJ_vDereferenceObject((POBJ)pPath)


#define PATH_IsPathOpen(dclevel) ( ((dclevel).hPath) && ((dclevel).flPath & DCPATH_ACTIVE) )

BOOL FASTCALL PATH_Arc (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines);
BOOL FASTCALL PATH_Ellipse (PDC dc, INT x1, INT y1, INT x2, INT y2);
VOID FASTCALL PATH_EmptyPath (PPATH pPath);
BOOL FASTCALL PATH_LineTo (PDC dc, INT x, INT y);
BOOL FASTCALL PATH_MoveTo (PDC dc);
BOOL FASTCALL PATH_PolyBezier (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyBezierTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyDraw(PDC dc, const POINT *pts, const BYTE *types, DWORD cbPoints);
BOOL FASTCALL PATH_Polygon (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_Polyline (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolylineTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons);
BOOL FASTCALL PATH_PolyPolyline( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines);
BOOL FASTCALL PATH_Rectangle (PDC dc, INT x1, INT y1, INT x2, INT y2);
BOOL FASTCALL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
BOOL FASTCALL PATH_PathToRegion (PPATH pPath, INT nPolyFillMode, PREGION Rgn);
BOOL FASTCALL PATH_ExtTextOut(PDC dc,INT x,INT y,UINT flags,const RECTL *lprc,LPCWSTR str,UINT count,const INT *dx);

BOOL FASTCALL PATH_AddEntry (PPATH pPath, const POINT *pPoint, BYTE flags);
BOOL FASTCALL PATH_AddFlatBezier (PPATH pPath, POINT *pt, BOOL closed);
BOOL FASTCALL PATH_DoArcPart (PPATH pPath, FLOAT_POINT corners[], double angleStart, double angleEnd, BYTE startEntryType);
BOOL FASTCALL PATH_FillPath( PDC dc, PPATH pPath );
BOOL FASTCALL PATH_FlattenPath (PPATH pPath);
VOID FASTCALL PATH_NormalizePoint (FLOAT_POINT corners[], const FLOAT_POINT *pPoint, double *pX, double *pY);

BOOL FASTCALL PATH_ReserveEntries (PPATH pPath, INT numEntries);
VOID FASTCALL PATH_ScaleNormalizedPoint (FLOAT_POINT corners[], double x, double y, POINT *pPoint);
BOOL FASTCALL PATH_StrokePath(DC *dc, PPATH pPath);
BOOL PATH_CheckCorners(DC *dc, POINT corners[], INT x1, INT y1, INT x2, INT y2);

VOID FASTCALL IntGdiCloseFigure(PPATH pPath);
BOOL FASTCALL PATH_Delete(HPATH hPath);

VOID FASTCALL IntGetCurrentPositionEx(PDC dc, LPPOINT pt);
