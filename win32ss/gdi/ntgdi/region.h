#pragma once

/* Type definitions ***********************************************************/

/* Internal region data.
   Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _REGION
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT BaseObject;
  _Notnull_ PRGN_ATTR prgnattr;
  RGN_ATTR rgnattr;

  RGNDATAHEADER rdh;
  RECTL *Buffer;
} REGION, *PREGION;

/* Globals ********************************************************************/

extern PREGION prgnDefault;
extern HRGN hrgnDefault;

/* Functions ******************************************************************/

PREGION FASTCALL REGION_AllocRgnWithHandle(INT n);
PREGION FASTCALL REGION_AllocUserRgnWithHandle(INT n);
VOID FASTCALL REGION_UnionRectWithRgn(PREGION rgn, const RECTL *rect);
INT FASTCALL REGION_SubtractRectFromRgn(PREGION prgnDest, PREGION prgnSrc, const RECTL *prcl);
INT FASTCALL REGION_GetRgnBox(PREGION Rgn, RECTL *pRect);
BOOL FASTCALL REGION_RectInRegion(PREGION Rgn, const RECTL *rc);
BOOL FASTCALL REGION_PtInRegion(PREGION, INT, INT);
INT FASTCALL REGION_CropRegion(PREGION rgnDst, PREGION rgnSrc, const RECTL *rect);
VOID FASTCALL REGION_SetRectRgn(PREGION pRgn, INT LeftRect, INT TopRect, INT RightRect, INT BottomRect);
VOID NTAPI REGION_vCleanup(PVOID ObjectBody);
VOID FASTCALL REGION_Delete(PREGION);
INT APIENTRY IntGdiGetRgnBox(HRGN, RECTL*);

PREGION
FASTCALL
REGION_LockRgn(
    _In_ HRGN hrgn);

VOID
FASTCALL
REGION_UnlockRgn(
    _In_ PREGION prgn);

BOOL
FASTCALL
REGION_bXformRgn(
    _Inout_ PREGION prgn,
    _In_ PMATRIX pmx);

BOOL
FASTCALL
REGION_SetPolyPolygonRgn(
    _Inout_ PREGION prgn,
    _In_ const POINT *ppt,
    _In_ const ULONG *pcPoints,
    _In_ ULONG cPolygons,
    _In_ INT iMode);

HRGN
NTAPI
GreCreatePolyPolygonRgn(
    _In_ const POINT *ppt,
    _In_ const ULONG *pcPoints,
    _In_ ULONG cPolygons,
    _In_ INT iMode);

BOOL
FASTCALL
REGION_bOffsetRgn(
    _Inout_ PREGION prgn,
    _In_ INT cx,
    _In_ INT cy);

BOOL FASTCALL IntRectInRegion(HRGN,LPRECTL);

INT FASTCALL IntGdiCombineRgn(PREGION, PREGION, PREGION, INT);
INT FASTCALL REGION_Complexity(PREGION);
PREGION FASTCALL IntSysCreateRectpRgn(INT,INT,INT,INT);
BOOL FASTCALL IntGdiSetRegionOwner(HRGN,DWORD);

HRGN
FASTCALL
GreCreateFrameRgn(
    HRGN hrgn,
    INT x,
    INT y);

#define IntSysCreateRectpRgnIndirect(prc) \
  IntSysCreateRectpRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

PREGION
FASTCALL
IntSysCreateRectpRgn(INT LeftRect, INT TopRect, INT RightRect, INT BottomRect);

// FIXME: move this
BOOL
FASTCALL
IntGdiPaintRgn(
    _In_ PDC pdc,
    _In_ PREGION prgn);
