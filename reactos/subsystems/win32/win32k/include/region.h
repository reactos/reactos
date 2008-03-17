
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

#define  REGION_FreeRgn(hRgn)  GDIOBJ_FreeObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)
#define  REGION_LockRgn(hRgn) ((PROSRGNDATA)GDIOBJ_LockObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION))
#define  REGION_UnlockRgn(pRgn) GDIOBJ_UnlockObjByPtr((POBJ)pRgn)

PROSRGNDATA FASTCALL REGION_AllocRgnWithHandle(INT n);
VOID FASTCALL REGION_UnionRectWithRgn(ROSRGNDATA *rgn, CONST RECT *rect);
INT FASTCALL REGION_GetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
BOOL FASTCALL REGION_RectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);
BOOL FASTCALL REGION_CropAndOffsetRegion(PROSRGNDATA rgnDst, PROSRGNDATA rgnSrc, const PRECT rect, const PPOINT off);
VOID FASTCALL REGION_SetRectRgn(PROSRGNDATA pRgn, INT LeftRect, INT TopRect, INT RightRect, INT BottomRect);
BOOL INTERNAL_CALL REGION_Cleanup(PVOID ObjectBody);

INT STDCALL IntGdiGetRgnBox(HRGN, LPRECT);
BOOL FASTCALL IntGdiPaintRgn(PDC, HRGN );
HRGN FASTCALL GdiCreatePolyPolygonRgn(CONST PPOINT, CONST PINT, INT, INT );
int FASTCALL IntGdiGetClipBox(HDC hDC, LPRECT rc);
INT STDCALL IntGdiSelectVisRgn(HDC hdc, HRGN hrgn);


#define UnsafeIntCreateRectRgnIndirect(prc) \
  NtGdiCreateRectRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

#endif /* not __WIN32K_REGION_H */
