#pragma once

/* Type definitions ***********************************************************/

/* Internal region data.
   Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _ROSRGNDATA
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT    BaseObject;
  PRGN_ATTR prgnattr;
  RGN_ATTR rgnattr;

  RGNDATAHEADER rdh;
  RECTL        *Buffer;
} ROSRGNDATA, *PROSRGNDATA, *LPROSRGNDATA, REGION, *PREGION;


/* Functions ******************************************************************/

#define  REGION_FreeRgn(pRgn)  GDIOBJ_FreeObj((POBJ)pRgn, GDIObjType_RGN_TYPE)
#define  REGION_FreeRgnByHandle(hRgn)  GDIOBJ_FreeObjByHandle((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)

PROSRGNDATA FASTCALL REGION_AllocRgnWithHandle(INT n);
PROSRGNDATA FASTCALL REGION_AllocUserRgnWithHandle(INT n);
VOID FASTCALL REGION_UnionRectWithRgn(ROSRGNDATA *rgn, const RECTL *rect);
INT FASTCALL REGION_SubtractRectFromRgn(PREGION prgnDest, PREGION prgnSrc, const RECTL *prcl);
INT FASTCALL REGION_GetRgnBox(PROSRGNDATA Rgn, RECTL *pRect);
BOOL FASTCALL REGION_RectInRegion(PROSRGNDATA Rgn, const RECTL *rc);
BOOL FASTCALL REGION_PtInRegion(PREGION, INT, INT);
INT FASTCALL REGION_CropAndOffsetRegion(PROSRGNDATA rgnDst, PROSRGNDATA rgnSrc, const RECTL *rect, const POINT *off);
VOID FASTCALL REGION_SetRectRgn(PROSRGNDATA pRgn, INT LeftRect, INT TopRect, INT RightRect, INT BottomRect);
VOID NTAPI REGION_vCleanup(PVOID ObjectBody);

extern PROSRGNDATA prgnDefault;
extern HRGN        hrgnDefault;

VOID FASTCALL REGION_Delete(PROSRGNDATA);
VOID FASTCALL IntGdiReleaseRaoRgn(PDC);
VOID FASTCALL IntGdiReleaseVisRgn(PDC);

INT APIENTRY IntGdiGetRgnBox(HRGN, RECTL*);
BOOL FASTCALL IntGdiPaintRgn(PDC, PREGION );
BOOL FASTCALL IntSetPolyPolygonRgn(PPOINT, PULONG, INT, INT, PREGION);
INT FASTCALL IntGdiOffsetRgn(PROSRGNDATA,INT,INT);
BOOL FASTCALL IntRectInRegion(HRGN,LPRECTL);

INT FASTCALL IntGdiCombineRgn(PROSRGNDATA, PROSRGNDATA, PROSRGNDATA, INT);
INT FASTCALL REGION_Complexity(PROSRGNDATA);
PROSRGNDATA FASTCALL RGNOBJAPI_Lock(HRGN,PRGN_ATTR *);
VOID FASTCALL RGNOBJAPI_Unlock(PROSRGNDATA);
PROSRGNDATA FASTCALL IntSysCreateRectpRgn(INT,INT,INT,INT);
BOOL FASTCALL IntGdiSetRegionOwner(HRGN,DWORD);

#define IntSysCreateRectpRgnIndirect(prc) \
  IntSysCreateRectpRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

PROSRGNDATA
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
