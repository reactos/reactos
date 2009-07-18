/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpoint.c
 * PURPOSE:         Pointer Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngMovePointer(
  IN SURFOBJ  *pso,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl)
{
    UNIMPLEMENTED;
}

ULONG
APIENTRY
EngSetPointerShape(
  IN SURFOBJ  *pso,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN LONG  xHot,
  IN LONG  yHot,
  IN LONG  x,
  IN LONG  y,
  IN RECTL  *prcl,
  IN FLONG  fl)
{
    UNIMPLEMENTED;
	return 0;
}

BOOL
APIENTRY
EngSetPointerTag(
  IN HDEV  hdev,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN FLONG  fl)
{
    UNIMPLEMENTED;
	return FALSE;
}
