/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engxlate.c
 * PURPOSE:         XLATEOBJ Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

ULONG
APIENTRY
XLATEOBJ_cGetPalette(
  IN XLATEOBJ  *pxlo,
  IN ULONG  iPal,
  IN ULONG  cPal,
  OUT ULONG  *pPal)
{
    UNIMPLEMENTED;
	return 0;
}

HANDLE
APIENTRY
XLATEOBJ_hGetColorTransform(
  IN XLATEOBJ  *pxlo)
{
    UNIMPLEMENTED;
	return NULL;
}

ULONG
APIENTRY
XLATEOBJ_iXlate(
  IN XLATEOBJ  *pxlo,
  IN ULONG  iColor)
{
    UNIMPLEMENTED;
	return 0;
}

PULONG
APIENTRY
XLATEOBJ_piVector(IN XLATEOBJ* XlateObj)
{
    UNIMPLEMENTED;
	return NULL;
}
