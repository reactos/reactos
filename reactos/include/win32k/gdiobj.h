/*
 *  GDI object common header definition
 * 
 * (RJJ) taken from WINE
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

#include <ddk/ntddk.h>

  /* GDI objects magic numbers */
#define GO_PEN_MAGIC             0x4f47
#define GO_BRUSH_MAGIC           0x4f48
#define GO_FONT_MAGIC            0x4f49
#define GO_PALETTE_MAGIC         0x4f4a
#define GO_BITMAP_MAGIC          0x4f4b
#define GO_REGION_MAGIC          0x4f4c
#define GO_DC_MAGIC              0x4f4d
#define GO_DISABLED_DC_MAGIC     0x4f4e
#define GO_META_DC_MAGIC         0x4f4f
#define GO_METAFILE_MAGIC        0x4f50
#define GO_METAFILE_DC_MAGIC     0x4f51
#define GO_ENHMETAFILE_MAGIC     0x4f52
#define GO_ENHMETAFILE_DC_MAGIC  0x4f53
#define GO_MAGIC_DONTCARE        0xffff

typedef struct _GDIOBJHDR
{
  HANDLE  hNext;
  WORD  wMagic;
  DWORD  dwCount;
  KSPIN_LOCK  Lock;
} GDIOBJHDR, *PGDIOBJHDR;

typedef PVOID PGDIOBJ;

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic);

#endif

