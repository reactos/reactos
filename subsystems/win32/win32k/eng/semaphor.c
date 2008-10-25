#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID )
{
  // www.osr.com/ddk/graphics/gdifncs_95lz.htm
  PERESOURCE psem = ExAllocatePoolWithTag( NonPagedPool, sizeof(ERESOURCE), TAG_GSEM );
  if ( !psem )
    return NULL;
  if ( !NT_SUCCESS(ExInitializeResourceLite ( psem )) )
  {
    ExFreePool ( psem );
    return NULL;
  }
  return (HSEMAPHORE)psem;
}

VOID
FASTCALL
IntGdiAcquireSemaphore ( HSEMAPHORE hsem )
{
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite ( (PERESOURCE)hsem, TRUE );
}

/*
 * @implemented
 */
VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_14br.htm
  PW32THREAD W32Thread;
  ASSERT(hsem);
  IntGdiAcquireSemaphore ( hsem );
  W32Thread = PsGetThreadWin32Thread(PsGetCurrentThread());
  if (W32Thread) W32Thread->dwEngAcquireCount++;
}


VOID
FASTCALL
IntGdiReleaseSemaphore ( HSEMAPHORE hsem )
{
  ExReleaseResourceLite ( (PERESOURCE)hsem );
  KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
  // www.osr.com/ddk/graphics/gdifncs_5u3r.htm
  PW32THREAD W32Thread;
  ASSERT(hsem);
  W32Thread = PsGetThreadWin32Thread(PsGetCurrentThread());
  if (W32Thread) --W32Thread->dwEngAcquireCount;
  IntGdiReleaseSemaphore ( hsem );
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

  ExDeleteResourceLite((PERESOURCE)hsem);

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

/*
 * @implemented
 */
BOOL STDCALL
EngInitializeSafeSemaphore(
   OUT ENGSAFESEMAPHORE *Semaphore)
{
   HSEMAPHORE hSem;

   if (InterlockedIncrement(&Semaphore->lCount) == 1)
   {
      /* Create the semaphore */
      hSem = EngCreateSemaphore();
      if (hSem == 0)
      {
         InterlockedDecrement(&Semaphore->lCount);
         return FALSE;
      }
      /* FIXME - not thread-safe! Check result of InterlockedCompareExchangePointer
                 and delete semaphore if already initialized! */
      (void)InterlockedExchangePointer((volatile PVOID *)&Semaphore->hsem, hSem);
   }
   else
   {
      /* Wait for the other thread to create the semaphore */
      ASSERT(Semaphore->lCount > 1);
      ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);
      while (Semaphore->hsem == NULL);
   }

   return TRUE;
}

/*
 * @implemented
 */
VOID STDCALL
EngDeleteSafeSemaphore(
   IN OUT ENGSAFESEMAPHORE *Semaphore)
{
   if (InterlockedDecrement(&Semaphore->lCount) == 0)
   {
      /* FIXME - not thread-safe! Use result of InterlockedCompareExchangePointer! */
      EngDeleteSemaphore(Semaphore->hsem);
      (void)InterlockedExchangePointer((volatile PVOID *)&Semaphore->hsem, NULL);
   }
}


