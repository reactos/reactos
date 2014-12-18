#pragma once

/* Type definitions ***********************************************************/

/* Internal region data.
   Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _REGION
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT BaseObject;
  PRGN_ATTR prgnattr;
  RGN_ATTR rgnattr;

  RGNDATAHEADER rdh;
  RECTL *Buffer;
} REGION, *PREGION;

/* Globals ********************************************************************/

extern PREGION prgnDefault;
extern HRGN hrgnDefault;

/* Functions ******************************************************************/

#define  REGION_FreeRgn(pRgn)  GDIOBJ_FreeObj((POBJ)pRgn, GDIObjType_RGN_TYPE)
#define  REGION_FreeRgnByHandle(hRgn)  GDIOBJ_FreeObjByHandle((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)

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

INT FASTCALL IntGdiOffsetRgn(PREGION,INT,INT);
BOOL FASTCALL IntRectInRegion(HRGN,LPRECTL);

INT FASTCALL IntGdiCombineRgn(PREGION, PREGION, PREGION, INT);
INT FASTCALL REGION_Complexity(PREGION);
PREGION FASTCALL RGNOBJAPI_Lock(HRGN,PRGN_ATTR *);
VOID FASTCALL RGNOBJAPI_Unlock(PREGION);
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

FORCEINLINE
PREGION
REGION_LockRgn(HRGN hrgn)
{
    return GDIOBJ_LockObject(hrgn, GDIObjType_RGN_TYPE);
}

FORCEINLINE
VOID
REGION_UnlockRgn(PREGION prgn)
{
    GDIOBJ_vUnlockObject(&prgn->BaseObject);
}

// FIXME: move this
BOOL FASTCALL IntGdiPaintRgn(PDC, PREGION );
