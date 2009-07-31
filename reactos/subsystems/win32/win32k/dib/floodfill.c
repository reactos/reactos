/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS win32 subsystem
 * PURPOSE:           Flood filling support
 * FILE:              subsystems/win32/win32k/dib/floodfill.c
 * PROGRAMMER:        Gregor Schneider, <grschneider AT gmail DOT com>
 */

#include <w32k.h>

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

static ULONG floodLen = 0;
static FLOODITEM *floodStart = NULL, *floodData = NULL;

static __inline BOOL initFlood(RECTL *DstRect)
{
    ULONG width = DstRect->right - DstRect->left;
    ULONG height = DstRect->bottom - DstRect->top;
    floodData = ExAllocatePoolWithTag(NonPagedPool, width * height * sizeof(FLOODITEM), TAG_DIB); 
    if (floodData == NULL)
    {
        return FALSE;
    }
    floodStart = (FLOODITEM*)((PBYTE)floodData + (width * height * sizeof(FLOODITEM)));
    return TRUE;
}
static __inline VOID finalizeFlood()
{
    ExFreePoolWithTag(floodData, TAG_DIB);
}
static __inline VOID addItemFlood(ULONG x, 
                                  ULONG y, 
                                  SURFOBJ *DstSurf, 
                                  RECTL *DstRect, 
                                  XLATEOBJ* ColorTranslation, 
                                  COLORREF Color, 
                                  BOOL isSurf)
{
    if (x >= DstRect->left && x <= DstRect->right &&
        y >= DstRect->top && y <= DstRect->bottom)
    {
        if (isSurf == TRUE && XLATEOBJ_iXlate(ColorTranslation, 
            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y)) != Color)
        {
            return;
        }
        else if (isSurf == FALSE && XLATEOBJ_iXlate(ColorTranslation, 
            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y)) == Color)
        {
            return;
        }
        floodStart--;
        floodStart->x = x;
        floodStart->y = y;
        floodLen++;
    }
}
static __inline VOID removeItemFlood()
{
    floodStart++;
    floodLen--;
}
static __inline BOOL isEmptyFlood()
{
    if (floodLen == 0)
    {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN DIB_XXBPP_FloodFill(SURFOBJ *DstSurf, 
                            BRUSHOBJ *Brush, 
                            RECTL *DstRect, 
                            POINTL *Origin,
                            XLATEOBJ *ColorTranslation,
                            COLORREF Color, 
                            UINT FillType)
{
    ULONG x, y;
    ULONG BrushColor;

    BrushColor = Brush->iSolidColor;
    x = Origin->x;
    y = Origin->y;

    if (FillType == FLOODFILLBORDER)
    {
        /* Check if the start pixel has the border color */
        if (XLATEOBJ_iXlate(ColorTranslation, 
            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y)) == Color)
        {
            return FALSE;
        }

        if (initFlood(DstRect) == FALSE)
        {
            return FALSE;
        }
        addItemFlood(x, y, DstSurf, DstRect, ColorTranslation, Color, FALSE);
        while (!isEmptyFlood()) 
        {
            x = floodStart->x;
            y = floodStart->y;
            removeItemFlood();

            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_PutPixel(DstSurf, x, y, BrushColor);
            addItemFlood(x, y + 1, DstSurf, DstRect, ColorTranslation, Color, FALSE);
            addItemFlood(x, y - 1, DstSurf, DstRect, ColorTranslation, Color, FALSE);
            addItemFlood(x + 1, y, DstSurf, DstRect, ColorTranslation, Color, FALSE);
            addItemFlood(x - 1, y, DstSurf, DstRect, ColorTranslation, Color, FALSE);
        }
        finalizeFlood();
    }
    else if (FillType == FLOODFILLSURFACE)
    {
        /* Check if the start pixel has the surface color */
        if (XLATEOBJ_iXlate(ColorTranslation, 
            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_GetPixel(DstSurf, x, y)) != Color)
        {
            return FALSE;
        }

        if (initFlood(DstRect) == FALSE)
        {
            return FALSE;
        }
        addItemFlood(x, y, DstSurf, DstRect, ColorTranslation, Color, TRUE);
        while (!isEmptyFlood()) 
        {
            x = floodStart->x;
            y = floodStart->y;
            removeItemFlood();

            DibFunctionsForBitmapFormat[DstSurf->iBitmapFormat].DIB_PutPixel(DstSurf, x, y, BrushColor);
            addItemFlood(x, y + 1, DstSurf, DstRect, ColorTranslation, Color, TRUE);
            addItemFlood(x, y - 1, DstSurf, DstRect, ColorTranslation, Color, TRUE);
            addItemFlood(x + 1, y, DstSurf, DstRect, ColorTranslation, Color, TRUE);
            addItemFlood(x - 1, y, DstSurf, DstRect, ColorTranslation, Color, TRUE);

        }
        finalizeFlood();
    }
    else
    {
        DPRINT1("Unsupported FloodFill type!\n");
        return FALSE;
    }
    return TRUE;
}
