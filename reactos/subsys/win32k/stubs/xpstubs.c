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

typedef PVOID EVENT, *PEVENT;

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

BOOL
STDCALL
EngCreateEvent ( OUT PEVENT *ppEvent )
{
  // www.osr.com/ddk/graphics/gdifncs_1civ.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
EngDeleteEvent ( IN PEVENT pEvent)
{
  // www.osr.com/ddk/graphics/gdifncs_6qp3.htm
  UNIMPLEMENTED;
  return FALSE;
}

PEVENT
STDCALL
EngMapEvent(
	IN HDEV    hDev,
	IN HANDLE  hUserObject,
	IN PVOID   Reserved1,
	IN PVOID   Reserved2,
	IN PVOID   Reserved3
	)
{
  // www.osr.com/ddk/graphics/gdifncs_3pnr.htm
  UNIMPLEMENTED;
  return FALSE;
}

LONG
STDCALL
EngSetEvent ( IN PEVENT pEvent )
{
  // www.osr.com/ddk/graphics/gdifncs_6p0n.htm
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
EngUnmapEvent ( IN PEVENT pEvent )
{
  // www.osr.com/ddk/graphics/gdifncs_5m7b.htm
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
EngWaitForSingleObject (
	IN PEVENT          pEvent,
	IN PLARGE_INTEGER  pTimeOut
	)
{
  // www.osr.com/ddk/graphics/gdifncs_4n53.htm
  UNIMPLEMENTED;
  return FALSE;
}
