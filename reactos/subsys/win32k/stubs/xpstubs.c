/*
 * Stubs for unimplemented WIN32K.SYS exports that are only available
 * in Windows XP and beyond ( i.e. a low priority for us right now )
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include <include/error.h>

#define STUB(x) void x(void) { DbgPrint("WIN32K: Stub for %s\n", #x); }

BOOL
STDCALL
EngAlphaBlend(
  IN SURFOBJ  *psoDest,
  IN SURFOBJ  *psoSrc,
  IN CLIPOBJ  *pco,
  IN XLATEOBJ  *pxlo,
  IN RECTL  *prclDest,
  IN RECTL  *prclSrc,
  IN BLENDOBJ  *pBlendObj)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
EngControlSprites(
  IN WNDOBJ  *pwo,
  IN FLONG  fl)
{
  UNIMPLEMENTED;
  return FALSE;
}

