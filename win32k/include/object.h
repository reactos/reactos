#ifndef _WIN32K_OBJECT_H
#define _WIN32K_OBJECT_H

#include <win32k/gdiobj.h>
#include <win32k/bitmaps.h>
#include <win32k/pen.h>


typedef struct _VOLATILE_OBJECT_ENTRY
{
   PUSER_OBJECT_HDR hdr;
   PUSER_HANDLE_ENTRY handle_entry;
   WORD recycle_count;
   WORD objType;
   SINGLE_LIST_ENTRY link;
} VOLATILE_OBJECT_ENTRY, *PVOLATILE_OBJECT_ENTRY;


typedef struct _OBJECT_ENTRY
{
   PUSER_OBJECT_HDR hdr;
   PUSER_HANDLE_ENTRY handle_entry;
   WORD generation;
   WORD type;
} OBJECT_ENTRY, *POBJECT_ENTRY;



#define USER_OBJ_DESTROYING         (0x1)
#define USER_OBJ_DESTROYED          (0x2)

VOID  INTERNAL_CALL InitGdiObjectHandleTable (VOID);
VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);


#endif /* _WIN32K_OBJECT_H */


/* EOF */
