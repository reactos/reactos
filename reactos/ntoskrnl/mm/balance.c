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
/* $Id: balance.c,v 1.2 2001/12/29 14:32:22 dwelch Exp $
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
  NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed, PVOID* FreedPages);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

/* GLOBALS ******************************************************************/

static MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiNrAvailablePages;
static ULONG MiNrTotalPages;

/* FUNCTIONS ****************************************************************/

VOID
MmInitializeBalancer(ULONG NrAvailablePages)
{
  memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));

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
					    PULONG NrFreed, PVOID* FreedPages))
{
  MiMemoryConsumers[Consumer].Trim = Trim;
}

NTSTATUS
MmReleasePageMemoryConsumer(ULONG Consumer, PVOID Page)
{
  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
  InterlockedIncrement(&MiNrAvailablePages);
  MmDereferencePage(Page);
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
      MiMemoryConsumers[Consumer].Trim(Target, 0, NULL, NULL);
    }
}

VOID
MiRebalanceMemoryConsumers(PVOID* Page)
{
  LONG Target;
  ULONG i;
  PVOID* FreedPages;
  ULONG NrFreedPages;
  ULONG TotalFreedPages;
  PVOID* OrigFreedPages;

  Target = MiMinimumAvailablePages - MiNrAvailablePages;
  if (Target < 0)
    {
      Target = 1;
    }

  OrigFreedPages = FreedPages = alloca(sizeof(PVOID) * Target);
  TotalFreedPages = 0;

  for (i = 0; i < MC_MAXIMUM && Target > 0; i++)
    {
      if (MiMemoryConsumers[i].Trim != NULL)
	{
	  MiMemoryConsumers[i].Trim(Target, 0, &NrFreedPages, FreedPages);
	  Target = Target - NrFreedPages;
	  FreedPages = FreedPages + NrFreedPages;
	  TotalFreedPages = TotalFreedPages + NrFreedPages;
	}
    }
  if (Target > 0)
    {
      KeBugCheck(0);
    }
  if (Page != NULL)
    {
      *Page = OrigFreedPages[0];
      i = 1;
    }
  else
    {
      i = 0;
    }
  for (; i < TotalFreedPages; i++)
    {
      MmDereferencePage(OrigFreedPages[i]);
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
      MiRebalanceMemoryConsumers(NULL);
    }

  /*
   * Actually allocate the page.
   */
  Page = MmAllocPage(0);
  if (Page == NULL)
    {
      /* Still not trimmed enough. */
      if (!CanWait)
	{
	  InterlockedIncrement(&MiNrAvailablePages);
	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}
      MiRebalanceMemoryConsumers(&Page);
    }
  *AllocatedPage = Page;

  return(STATUS_SUCCESS);
}
