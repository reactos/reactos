/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/remlock.c
 * PURPOSE:         Remove Lock functions
 * 
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
IoInitializeRemoveLockEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN ULONG AllocateTag,
  IN ULONG MaxLockedMinutes,
  IN ULONG HighWatermark,
  IN ULONG RemlockSize)
{
  DPRINT("IoInitializeRemoveLockEx called");
  RtlZeroMemory(RemoveLock, RemlockSize);
  RemoveLock->Common.IoCount = 1;
  KeInitializeEvent(&RemoveLock->Common.RemoveEvent, NotificationEvent, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoAcquireRemoveLockEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN OPTIONAL PVOID Tag,
  IN LPCSTR File,
  IN ULONG Line,
  IN ULONG RemlockSize)
{
  DPRINT("IoAcquireRemoveLockEx called");
  InterlockedIncrement(&RemoveLock->Common.IoCount);
  if (RemoveLock->Common.Removed)
  {
    if (InterlockedDecrement(&RemoveLock->Common.IoCount) == 0)
    {
      KeSetEvent(&RemoveLock->Common.RemoveEvent, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_DELETE_PENDING;
  }
  return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
IoReleaseRemoveLockEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN PVOID Tag,
  IN ULONG RemlockSize)
{
  LONG IoCount;

  DPRINT("IoReleaseRemoveLockEx called");
  IoCount = InterlockedDecrement(&RemoveLock->Common.IoCount);
  if (IoCount == 0)
  {
    KeSetEvent(&RemoveLock->Common.RemoveEvent, IO_NO_INCREMENT, FALSE);
  }
}

/*
 * @implemented
 */
VOID
STDCALL
IoReleaseRemoveLockAndWaitEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN PVOID Tag,
  IN ULONG RemlockSize)
{
  DPRINT("IoReleaseRemoveLockAndWaitEx called");
  RemoveLock->Common.Removed = TRUE;
  InterlockedDecrement(&RemoveLock->Common.IoCount);
  IoReleaseRemoveLockEx(RemoveLock, Tag, RemlockSize);
  KeWaitForSingleObject(&RemoveLock->Common.RemoveEvent, Executive, KernelMode,
    FALSE, NULL);
}

/* EOF */
