
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <win32k/debug1.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include <include/error.h>

/*
 * @implemented
 */
HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_95lz.htm
  PERESOURCE psem = ExAllocatePool ( NonPagedPool, sizeof(ERESOURCE) );
  if ( !psem )
    return NULL;
  if ( !NT_SUCCESS(ExInitializeResourceLite ( psem )) )
  {
    ExFreePool ( psem );
    return NULL;
  }
  return (HSEMAPHORE)psem;
}

/*
 * @implemented
 */
VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_14br.htm
  ASSERT(hsem);
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite ( (PERESOURCE)hsem, TRUE );
}

/*
 * @implemented
 */
VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_5u3r.htm
  ASSERT(hsem);
  ExReleaseResourceLite ( (PERESOURCE)hsem );
  KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_13c7.htm
  ASSERT ( hsem );
  ExFreePool ( (PVOID)hsem );
}

/*
 * @implemented
 */
BOOL
STDCALL
EngIsSemaphoreOwned ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_6wmf.htm
  ASSERT(hsem);
  return (((PERESOURCE)hsem)->ActiveCount > 0);
}

/*
 * @implemented
 */
BOOL
STDCALL
EngIsSemaphoreOwnedByCurrentThread ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_9yxz.htm
  ASSERT(hsem);
  return ExIsResourceAcquiredExclusiveLite ( (PERESOURCE)hsem );
}
