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

// Path type flags
#define PATHTYPE_KEEPME 1
#define PATHTYPE_STACK  2

/* extended PATHDATA */
typedef struct _EXTPATHDATA
{
    PATHDATA pd;
    struct _EXTPATHDATA *ppdNext;
} EXTPATHDATA, *PEXTPATHDATA;

typedef struct _PATH
{
  BASEOBJECT   BaseObject;
  //PVOID        ppachain;
  RECTFX       rcfxBoundBox;
  POINTFX      ptfxSubPathStart;
  FLONG        flType;
  PEXTPATHDATA ppdFirst;
  PEXTPATHDATA ppdLast;
  FLONG        flags;   // PATHDATA flags.
  PEXTPATHDATA ppdCurrent;
  // PATHOBJ;
  FLONG        fl;      // Saved flags.
  ULONG        cCurves; // Saved number of lines and Bezier.

  struct _EPATHOBJ *epo;

  // Wine/ReactOS Things to convert from:
  FLONG        state;
  POINT        *pPoints;
  BYTE         *pFlags;
  int          numEntriesUsed;
  int          numEntriesAllocated;
  BOOL         newStroke;
  POINT        pos;
} PATH, *PPATH;

typedef struct _EPATHOBJ
{
  PATHOBJ  po;
  PPATH    pPath;
  CLIPOBJ *pco;
} EPATHOBJ, *PEPATHOBJ;

#define  PATH_AllocPath() ((PPATH) GDIOBJ_AllocObj(GDIObjType_PATH_TYPE))
#define  PATH_AllocPathWithHandle() ((PPATH) GDIOBJ_AllocObjWithHandle (GDI_OBJECT_TYPE_PATH, sizeof(PATH)))
#define  PATH_LockPath(hPath) ((PPATH)GDIOBJ_ShareLockObj((HGDIOBJ)hPath, GDI_OBJECT_TYPE_PATH))
#define  PATH_UnlockPath(pPath) GDIOBJ_vDereferenceObject((POBJ)pPath)
#define PATH_IsPathOpen(dclevel) ( ((dclevel).hPath) && ((dclevel).flPath & DCPATH_ACTIVE) )

BOOL FASTCALL PATH_Arc (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd, INT direction, INT lines);
BOOL PATH_Ellipse (PDC dc, INT x1, INT y1, INT x2, INT y2);
PPATH FASTCALL PATH_CreatePath(int count);
VOID FASTCALL PATH_EmptyPath (PPATH pPath);
BOOL FASTCALL PATH_LineTo (PDC dc, INT x, INT y);
BOOL FASTCALL PATH_MoveTo(PDC dc, PPATH pPath);
BOOL FASTCALL PATH_PolyBezier (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyBezierTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyDraw(PDC dc, const POINT *pts, const BYTE *types, DWORD cbPoints);
BOOL FASTCALL PATH_PolylineTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons);
BOOL FASTCALL PATH_PolyPolyline( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines);
BOOL FASTCALL PATH_Rectangle (PDC dc, INT x1, INT y1, INT x2, INT y2);
BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
BOOL FASTCALL PATH_PathToRegion (PPATH pPath, INT nPolyFillMode, PREGION Rgn);
BOOL FASTCALL PATH_ExtTextOut(PDC dc,INT x,INT y,UINT flags,const RECTL *lprc,LPCWSTR str,UINT count,const INT *dx);

BOOL FASTCALL PATH_AddEntry (PPATH pPath, const POINT *pPoint, BYTE flags);
BOOL FASTCALL PATH_AddFlatBezier (PPATH pPath, POINT *pt, BOOL closed);
BOOL FASTCALL PATH_FillPath( PDC dc, PPATH pPath );
BOOL FASTCALL PATH_FillPathEx(PDC dc, PPATH pPath, PBRUSH pbrFill);
PPATH FASTCALL PATH_FlattenPath (PPATH pPath);
PPATH FASTCALL PATH_WidenPathEx(DC *dc, PPATH pPath);

BOOL FASTCALL PATH_ReserveEntries (PPATH pPath, INT numEntries);
BOOL FASTCALL PATH_StrokePath(DC *dc, PPATH pPath);

VOID FASTCALL IntGdiCloseFigure(PPATH pPath);
BOOL FASTCALL PATH_Delete(HPATH hPath);

VOID FASTCALL IntGetCurrentPositionEx(PDC dc, LPPOINT pt);

BOOL PATH_RestorePath( DC *, DC *);
BOOL PATH_SavePath( DC *, DC *);
BOOL IntGdiFillRgn(PDC pdc, PREGION prgn, PBRUSH pbrFill);
PPATH FASTCALL
IntGdiWidenPath(PPATH pPath, UINT penWidth, UINT penStyle, FLOAT eMiterLimit);
