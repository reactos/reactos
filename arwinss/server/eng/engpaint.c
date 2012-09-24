/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpaint.c
 * PURPOSE:         Miscellaneous Painting Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngGradientFill(
  IN SURFOBJ  *psoDest,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN TRIVERTEX  *pVertex,
  IN ULONG  nVertex,
  IN PVOID  pMesh,
  IN ULONG  nMesh,
  IN RECTL  *prclExtents,
  IN POINTL  *pptlDitherOrg,
  IN ULONG  ulMode)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngPaint(IN SURFOBJ  *pso,
         IN CLIPOBJ  *pco,
         IN BRUSHOBJ  *pbo,
         IN POINTL  *pptlBrushOrg,
         IN MIX  mix)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOLEAN
APIENTRY
EngNineGrid(IN SURFOBJ* DestSurf,
            IN SURFOBJ* SourceSurf,
            IN CLIPOBJ* ClipObj,
            IN XLATEOBJ* XlateObj,
            IN RECTL* prclSource,
            IN RECTL* prclDest,
            PVOID pvUnknown1,
            PVOID pvUnknown2,
            DWORD dwReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
