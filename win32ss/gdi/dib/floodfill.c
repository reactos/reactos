/*
* COPYRIGHT:         See COPYING in the top level directory
* PROJECT:           ReactOS win32 subsystem
* PURPOSE:           Flood filling support
* FILE:              win32ss/gdi/dib/floodfill.c
* PROGRAMMER:        Gregor Schneider <grschneider AT gmail DOT com>
*/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
*  This floodfill algorithm is an iterative four neighbors version. It works with an internal stack like data structure.
*  The stack is kept in an array, sized for the worst case scenario of having to add all pixels of the surface.
*  This avoids having to allocate and free memory blocks  all the time. The stack grows from the end of the array towards the start.
*  All pixels are checked before being added, against belonging to the fill rule (FLOODFILLBORDER or FLOODFILLSURFACE)
*  and the position in respect to the clip region. This guarantees all pixels lying on the stack belong to the filled surface.
*  Further optimisations of the algorithm are possible.
*/

/* Floodfil helper structures and functions */
typedef struct _floodItem
{
  ULONG x;
  ULONG y;
} FLOODITEM;

typedef struct _floodInfo
{
  ULONG floodLen;
  FLOODITEM *floodStart;
  FLOODITEM *floodData;
} FLOODINFO;

static __inline BOOL initFlood(FLOODINFO *info, RECTL *DstRect)
{
  ULONG width = DstRect->right - DstRect->left;
  ULONG height = DstRect->bottom - DstRect->top;
  info->floodData = ExAllocatePoolWithTag(NonPagedPool, width * height * sizeof(FLOODITEM), TAG_DIB);
  if (info->floodData == NULL)
  {
    return FALSE;
  }
  info->floodStart = info->floodData + (width * height);
  DPRINT("Allocated flood stack from %p to %p\n", info->floodData, info->floodStart);
  return TRUE;
}
static __inline VOID finalizeFlood(FLOODINFO *info)
{
  ExFreePoolWithTag(info->floodData, TAG_DIB);
}
static __inline VOID addItemFlood(FLOODINFO *info,
                                  ULONG x,
                                  ULONG y,
                                  SURFOBJ *DstSurf,
                                  RECTL *DstRect,
                                  ULONG Color,
                                  BOOL isSurf)
{
  if (RECTL_bPointInRect(DstRect,x,y))
  {
    if (isSurf &&
      DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y) != Color)
    {
      return;
    }
    else if (isSurf == FALSE &&
      DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y) == Color)
    {
      return;
    }
    info->floodStart--;
    info->floodStart->x = x;
    info->floodStart->y = y;
    info->floodLen++;
  }
}
static __inline VOID removeItemFlood(FLOODINFO *info)
{
  info->floodStart++;
  info->floodLen--;
}

BOOLEAN DIB_XXBPP_FloodFillSolid(SURFOBJ *DstSurf,
                                 BRUSHOBJ *Brush,
                                 RECTL *DstRect,
                                 POINTL *Origin,
                                 ULONG ConvColor,
                                 UINT FillType)
{
  ULONG x, y;
  ULONG BrushColor;
  FLOODINFO flood = {0, NULL, NULL};

  BrushColor = Brush->iSolidColor;
  x = Origin->x;
  y = Origin->y;

  if (FillType == FLOODFILLBORDER)
  {
    /* Check if the start pixel has the border color */
    if (DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y) == ConvColor)
    {
      return FALSE;
    }

    if (initFlood(&flood, DstRect) == FALSE)
    {
      return FALSE;
    }
    addItemFlood(&flood, x, y, DstSurf, DstRect, ConvColor, FALSE);
    while (flood.floodLen != 0)
    {
      x = flood.floodStart->x;
      y = flood.floodStart->y;
      removeItemFlood(&flood);

      DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_PutPixel(DstSurf, x, y, BrushColor);
      if (flood.floodStart - 4 < flood.floodData)
      {
        DPRINT1("Can't finish flooding!\n");
        finalizeFlood(&flood);
        return FALSE;
      }
      addItemFlood(&flood, x, y + 1, DstSurf, DstRect, ConvColor, FALSE);
      addItemFlood(&flood, x, y - 1, DstSurf, DstRect, ConvColor, FALSE);
      addItemFlood(&flood, x + 1, y, DstSurf, DstRect, ConvColor, FALSE);
      addItemFlood(&flood, x - 1, y, DstSurf, DstRect, ConvColor, FALSE);
    }
    finalizeFlood(&flood);
  }
  else if (FillType == FLOODFILLSURFACE)
  {
    /* Check if the start pixel has the surface color */
    if (DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y) != ConvColor)
    {
      return FALSE;
    }

    if (initFlood(&flood, DstRect) == FALSE)
    {
      return FALSE;
    }
    addItemFlood(&flood, x, y, DstSurf, DstRect, ConvColor, TRUE);
    while (flood.floodLen != 0)
    {
      x = flood.floodStart->x;
      y = flood.floodStart->y;
      removeItemFlood(&flood);

      DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_PutPixel(DstSurf, x, y, BrushColor);
      if (flood.floodStart - 4 < flood.floodData)
      {
        DPRINT1("Can't finish flooding!\n");
        finalizeFlood(&flood);
        return FALSE;
      }
      addItemFlood(&flood, x, y + 1, DstSurf, DstRect, ConvColor, TRUE);
      addItemFlood(&flood, x, y - 1, DstSurf, DstRect, ConvColor, TRUE);
      addItemFlood(&flood, x + 1, y, DstSurf, DstRect, ConvColor, TRUE);
      addItemFlood(&flood, x - 1, y, DstSurf, DstRect, ConvColor, TRUE);
    }
    finalizeFlood(&flood);
  }
  else
  {
    DPRINT1("Unsupported FloodFill type!\n");
    return FALSE;
  }
  return TRUE;
}
