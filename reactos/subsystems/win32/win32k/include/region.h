
#ifndef __WIN32K_REGION_H
#define __WIN32K_REGION_H

#include "gdiobj.h"

/* Internal region data. Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _ROSRGNDATA {
  HGDIOBJ     hHmgr;
  PVOID       pvEntry;
  ULONG       lucExcLock;
  ULONG       Tid;

  RGNDATAHEADER rdh;
  PRECT         Buffer;
} ROSRGNDATA, *PROSRGNDATA, *LPROSRGNDATA;


#define  RGNDATA_FreeRgn(hRgn)  GDIOBJ_FreeObj(GdiHandleTable, (HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)
#define  RGNDATA_LockRgn(hRgn) ((PROSRGNDATA)GDIOBJ_LockObj(GdiHandleTable, (HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION))
#define  RGNDATA_UnlockRgn(pRgn) GDIOBJ_UnlockObjByPtr(GdiHandleTable, pRgn)
HRGN FASTCALL RGNDATA_AllocRgn(INT n);
BOOL INTERNAL_CALL RGNDATA_Cleanup(PVOID ObjectBody);

BOOL FASTCALL IntGdiPaintRgn(PDC, HRGN );
HRGN FASTCALL GdiCreatePolyPolygonRgn(CONST PPOINT, CONST PINT, INT, INT );

#endif

