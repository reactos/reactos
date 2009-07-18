#ifndef __WIN32K_CLIPOBJ_H
#define __WIN32K_CLIPOBJ_H

#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

typedef struct tagSPAN
{
  LONG Y;
  LONG X;
  ULONG Width;
} SPAN, *PSPAN;

typedef struct _CLIPGDI
{
  CLIPOBJ ClipObj;
  ULONG EnumPos;
  ULONG EnumOrder;
  ULONG EnumMax;
  ENUMRECTS EnumRects;
} CLIPGDI, *PCLIPGDI;

/* as the *OBJ structures are located at the beginning of the *GDI structures
   we can simply typecast the pointer */
#define ObjToGDI(ClipObj, Type) (Type##GDI *)(ClipObj)
#define GDIToObj(ClipGDI, Type) (Type##OBJ *)(ClipGDI)

CLIPOBJ* FASTCALL
IntEngCreateClipRegion(ULONG count, PRECTL pRect, PRECTL rcBounds);

VOID FASTCALL
IntEngDeleteClipRegion(CLIPOBJ *ClipObj);

#endif
