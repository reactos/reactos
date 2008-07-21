/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engxform.c
 * PURPOSE:         XFORMOBJ Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
XFORMOBJ_bApplyXform(
  IN XFORMOBJ  *pxo,
  IN ULONG  iMode,
  IN ULONG  cPoints,
  IN PVOID  pvIn,
  OUT PVOID  pvOut)
{
    UNIMPLEMENTED;
	return FALSE;
}

ULONG
APIENTRY
XFORMOBJ_iGetFloatObjXform(
  IN XFORMOBJ  *pxo,
  OUT FLOATOBJ_XFORM  *pxfo)
{
    UNIMPLEMENTED;
	return 0;
}

ULONG
APIENTRY
XFORMOBJ_iGetXform(
  IN XFORMOBJ  *pxo,
  OUT XFORML  *pxform)
{
    UNIMPLEMENTED;
	return 0;
}
