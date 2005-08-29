#ifndef _WIN32K_OBJECT_H
#define _WIN32K_OBJECT_H

#include <win32k/gdiobj.h>
#include <win32k/bitmaps.h>
#include <win32k/pen.h>


#define USER_OBJ_DESTROYING         (0x1)
#define USER_OBJ_DESTROYED          (0x2)

VOID  INTERNAL_CALL InitGdiObjectHandleTable (VOID);
VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);


#endif /* _WIN32K_OBJECT_H */


/* EOF */
