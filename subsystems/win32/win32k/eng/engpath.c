/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpath.c
 * PURPOSE:         Path Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

PATHOBJ*
APIENTRY
EngCreatePath(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngDeletePath(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix,
  IN FLONG  flOptions)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngStrokeAndFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pboStroke,
  IN LINEATTRS  *plineattrs,
  IN BRUSHOBJ  *pboFill,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mixFill,
  IN FLONG  flOptions)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngStrokePath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN LINEATTRS  *plineattrs,
  IN MIX  mix)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngLineTo(
  SURFOBJ  *pso,
  CLIPOBJ  *pco,
  BRUSHOBJ  *pbo,
  LONG  x1,
  LONG  y1,
  LONG  x2,
  LONG  y2,
  RECTL  *prclBounds,
  MIX  mix)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bCloseFigure(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnum(IN PATHOBJ  *ppo,
              OUT PATHDATA  *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnumClipLines(IN PATHOBJ  *ppo,
                       IN ULONG  cb,
                       OUT CLIPLINE  *pcl)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bMoveTo(IN PATHOBJ  *ppo,
                IN POINTFIX  ptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyBezierTo(IN PATHOBJ  *ppo,
                      IN POINTFIX  *pptfx,
                      IN ULONG  cptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyLineTo(IN PATHOBJ  *ppo,
                    IN POINTFIX  *pptfx,
                    IN ULONG  cptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

VOID
APIENTRY
PATHOBJ_vEnumStart(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vEnumStartClipLines(IN PATHOBJ  *ppo,
                            IN CLIPOBJ  *pco,
                            IN SURFOBJ  *pso,
                            IN LINEATTRS  *pla)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vGetBounds(IN PATHOBJ  *ppo,
                   OUT PRECTFX  prectfx)
{
    UNIMPLEMENTED;
}
