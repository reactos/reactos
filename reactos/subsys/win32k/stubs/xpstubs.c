/*
 * Stubs for unimplemented WIN32K.SYS exports that are only available
 * in Windows XP and beyond ( i.e. a low priority for us right now )
 */

#include <windows.h>
#include <ddk/ntddk.h>

#define STUB(x) void x(void) { DbgPrint("WIN32K: Stub for %s\n", #x); }

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
