
#ifndef __WIN32K_REGION_H
#define __WIN32K_REGION_H

#include "gdiobj.h"

/* Type definitions ***********************************************************/

/* Internal region data.
   Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _ROSRGNDATA
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT    BaseObject;

  RGNDATAHEADER rdh;
  PRECT         Buffer;
} ROSRGNDATA, *PROSRGNDATA, *LPROSRGNDATA;


/* Functions ******************************************************************/

#define  REGION_FreeRgn(pRgn)  GDIOBJ_FreeObj((POBJ)pRgn, GDIObjType_RGN_TYPE)
#define  REGION_FreeRgnByHandle(hRgn)  GDIOBJ_FreeObjByHandle((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)
#define  REGION_LockRgn(hRgn) ((PROSRGNDATA)GDIOBJ_LockObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION))
#define  REGION_UnlockRgn(pRgn) GDIOBJ_UnlockObjByPtr((POBJ)pRgn)

PROSRGNDATA FASTCALL REGION_AllocRgnWithHandle(INT n);
VOID FASTCALL REGION_UnionRectWithRgn(ROSRGNDATA *rgn, CONST RECT *rect);
INT FASTCALL REGION_GetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
BOOL FASTCALL REGION_RectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);
BOOL FASTCALL REGION_CropAndOffsetRegion(PROSRGNDATA rgnDst, PROSRGNDATA rgnSrc, const PRECT rect, const PPOINT off);
VOID FASTCALL REGION_SetRectRgn(PROSRGNDATA pRgn, INT LeftRect, INT TopRect, INT RightRect, INT BottomRect);
BOOL INTERNAL_CALL REGION_Cleanup(PVOID ObjectBody);

extern PROSRGNDATA prgnDefault;
extern HRGN        hrgnDefault;

VOID FASTCALL REGION_Delete(PROSRGNDATA);
VOID FASTCALL IntGdiReleaseRaoRgn(PDC);
VOID FASTCALL IntGdiReleaseVisRgn(PDC);

INT APIENTRY IntGdiGetRgnBox(HRGN, LPRECT);
BOOL FASTCALL IntGdiPaintRgn(PDC, HRGN );
HRGN FASTCALL IntCreatePolyPolygonRgn(PPOINT, PULONG, INT, INT);

INT FASTCALL IntGdiCombineRgn(PROSRGNDATA, PROSRGNDATA, PROSRGNDATA, INT);
INT FASTCALL REGION_Complexity(PROSRGNDATA);
PROSRGNDATA FASTCALL IntGdiCreateRectRgn(INT, INT, INT, INT);

#define UnsafeIntCreateRectRgnIndirect(prc) \
  NtGdiCreateRectRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

#endif /* not __WIN32K_REGION_H */
