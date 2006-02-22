#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
ULONG
STDCALL
EngGetLastError ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_3non.htm
  return GetLastNtError();
}

/*
 * @implemented
 */
VOID
STDCALL
EngSetLastError ( IN ULONG iError )
{
  // www.osr.com/ddk/graphics/gdifncs_95m0.htm
  SetLastNtError ( iError );
}
