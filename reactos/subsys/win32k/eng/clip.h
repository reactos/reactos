#ifndef __WIN32K_CLIP_H
#define __WIN32K_CLIP_H

typedef ULONG HCLIP;
CLIPOBJ * IntEngCreateClipRegion( ULONG count, PRECTL pRect, RECTL rcBounds );
VOID IntEngDeleteClipRegion(CLIPOBJ *ClipObj);


#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

#endif
