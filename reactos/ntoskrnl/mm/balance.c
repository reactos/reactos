/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: balance.c,v 1.3 2001/12/31 01:53:45 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/balance.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _MM_MEMORY_CONSUMER
{
  ULONG PagesUsed;
  ULONG PagesTarget;
  NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

typedef struct _MM_ALLOCATION_REQUEST
{
  PVOID Page;
  LIST_ENTRY ListEntry;
  KEVENT Event;
} MM_ALLOCATION_REQUEST, *PMM_ALLOCATION_REQUEST;

/* GLOBALS ******************************************************************/

static MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiNrAvailablePages;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;

/* FUNCTIONS ****************************************************************/

VOID
MmInitializeBalancer(ULONG NrAvailablePages)
{
  memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));
  InitializeListHead(&AllocationListHead);
  KeInitializeSpinLock(&AllocationListLock);

  MiNrAvailablePages = MiNrTotalPages = NrAvailablePages;

  /* Set up targets. */
  MiMinimumAvailablePages = 64;
  MiMemoryConsumers[MC_CACHE].PagesTarget = NrAvailablePages / 2;
  MiMemoryConsumers[MC_USER].PagesTarget = NrAvailablePages - MiMinimumAvailablePages;
  MiMemoryConsumers[MC_PPOOL].PagesTarget = NrAvailablePages / 2;
  MiMemoryConsumers[MC_NPPOOL].PagesTarget = 0xFFFFFFFF;
}

VOID
MmInitializeMemoryConsumer(ULONG Consumer, 
			   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, 
					    PULONG NrFreed))
{
  MiMemoryConsumers[Consumer].Trim = Trim;
}

NTSTATUS
MmReleasePageMemoryConsumer(ULONG Consumer, PVOID Page)
{
  PMM_ALLOCATION_REQUEST Request;
  PLIST_ENTRY Entry;
  KIRQL oldIrql;

  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
  InterlockedIncrement(&MiNrAvailablePages);
  KeAcquireSpinLock(&AllocationListLock, &oldIrql);
  if (IsListEmpty(&AllocationListHead))
    {
      KeReleaseSpinLock(&AllocationListLock, oldIrql);
      MmDereferencePage(Page);
    }
  else
    {
      Entry = RemoveHeadList(&AllocationListHead);
      Request = CONTAINING_RECORD(Entry, MM_ALLOCATION_REQUEST, ListEntry);
      KeReleaseSpinLock(&AllocationListLock, oldIrql);
      Request->Page = Page;
      KeSetEvent(&Request->Event, IO_NO_INCREMENT, FALSE);
    }
  return(STATUS_SUCCESS);
}

VOID
MiTrimMemoryConsumer(ULONG Consumer)
{
  LONG Target;

  Target = MiMemoryConsumers[Consumer].PagesUsed - MiMemoryConsumers[Consumer].PagesTarget;
  if (Target < 0)
    {
      Target = 1;
    }

  if (MiMemoryConsumers[Consumer].Trim != NULL)
    {
      MiMemoryConsumers[Consumer].Trim(Target, 0, NULL);
    }
}

VOID
MiRebalanceMemoryConsumers(VOID)
{
  LONG Target;
  ULONG i;
  ULONG NrFreedPages;
  NTSTATUS Status;

  Target = MiMinimumAvailablePages - MiNrAvailablePages;
  if (Target < 0)
    {
      Target = 1;
    }

  for (i = 0; i < MC_MAXIMUM && Target > 0; i++)
    {
      if (MiMemoryConsumers[i].Trim != NULL)
	{
	  Status = MiMemoryConsumers[i].Trim(Target, 0, &NrFreedPages);
	  if (!NT_SUCCESS(Status))
	    {
	      KeBugCheck(0);
	    }
	  Target = Target - NrFreedPages;
	}
    }
  if (Target > 0)
    {
      KeBugCheck(0);
    }
}

NTSTATUS
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait, PVOID* AllocatedPage)
{
  ULONG OldUsed;
  ULONG OldAvailable;
  PVOID Page;
  
  /*
   * Make sure we don't exceed our individual target.
   */
  OldUsed = InterlockedIncrement(&MiMemoryConsumers[Consumer].PagesUsed);
  if (OldUsed >= (MiMemoryConsumers[Consumer].PagesTarget - 1))
    {
      if (!CanWait)
	{
	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}
      MiTrimMemoryConsumer(Consumer);
    }

  /*
   * Make sure we don't exceed global targets.
   */
  OldAvailable = InterlockedDecrement(&MiNrAvailablePages);
  if (OldAvailable < MiMinimumAvailablePages)
    {
      if (!CanWait)
	{
	  InterlockedIncrement(&MiNrAvailablePages);
	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}
      MiRebalanceMemoryConsumers();
    }

  /*
   * Actually allocate the page.
   */
  Page = MmAllocPage(Consumer, 0);
  if (Page == NULL)
    {
      MM_ALLOCATION_REQUEST Request;
      KIRQL oldIrql;

      /* Still not trimmed enough. */
      if (!CanWait)
	{
	  InterlockedIncrement(&MiNrAvailablePages);
	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}

      /* Insert an allocation request. */
      Request.Page = NULL;
      KeInitializeEvent(&Request.Event, NotificationEvent, FALSE);
      KeAcquireSpinLock(&AllocationListLock, &oldIrql);
      InsertTailList(&AllocationListHead, &Request.ListEntry);
      KeReleaseSpinLock(&AllocationListLock, oldIrql);
      MiRebalanceMemoryConsumers();
      Page = Request.Page;
      if (Page == NULL)
	{
	  KeBugCheck(0);
	}
    }
  *AllocatedPage = Page;

  return(STATUS_SUCCESS);
}
