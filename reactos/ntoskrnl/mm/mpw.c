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
/* $Id: mpw.c,v 1.12 2003/01/11 15:47:14 hbirr Exp $
 *
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mpw.c
 * PURPOSE:      Writes data that has been modified in memory but not on
 *               the disk
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/mm.h>
#include <internal/cc.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static HANDLE MpwThreadHandle;
static CLIENT_ID MpwThreadId;
static KEVENT MpwThreadEvent;
static volatile BOOLEAN MpwThreadShouldTerminate;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MmWriteDirtyPages(ULONG Target, PULONG Actual)
{
  PHYSICAL_ADDRESS Page;
  PHYSICAL_ADDRESS NextPage;
  NTSTATUS Status;

  Page = MmGetLRUFirstUserPage();
  while (Page.QuadPart != 0LL && Target > 0)
    {
      /*
       * FIXME: While the current page is write back it is possible
       *        that the next page is freed and not longer a user page.
       */
      NextPage = MmGetLRUNextUserPage(Page);
      if (MmIsDirtyPageRmap(Page))
	{
	  Status = MmWritePagePhysicalAddress(Page);
	  if (NT_SUCCESS(Status))
	    {
	      Target--;
	    }
	} 
      Page = NextPage;
    }
  *Actual = Target;
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
MmMpwThreadMain(PVOID Ignored)
{
  NTSTATUS Status;
  ULONG PagesWritten;
  LARGE_INTEGER Timeout;
  
  Timeout.QuadPart = -50000000;
  
  for(;;)
    {
      Status = KeWaitForSingleObject(&MpwThreadEvent,
				     0,
				     KernelMode,
				     FALSE,
				     &Timeout);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("MpwThread: Wait failed\n");
	  KeBugCheck(0);
	  return(STATUS_UNSUCCESSFUL);
	}
      if (MpwThreadShouldTerminate)
	{
	  DbgPrint("MpwThread: Terminating\n");
	  return(STATUS_SUCCESS);
	}
      
      PagesWritten = 0;
#if 0
      /* 
       *  FIXME: MmWriteDirtyPages doesn't work correctly.
       */
      MmWriteDirtyPages(128, &PagesWritten);
#endif
      CcRosFlushDirtyPages(128, &PagesWritten);
    }
}

NTSTATUS MmInitMpwThread(VOID)
{
  KPRIORITY Priority;
  NTSTATUS Status;
  
  MpwThreadShouldTerminate = FALSE;
  KeInitializeEvent(&MpwThreadEvent, SynchronizationEvent, FALSE);
  
  Status = PsCreateSystemThread(&MpwThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&MpwThreadId,
				MmMpwThreadMain,
				NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
     }
  
  Priority = 1;
  NtSetInformationThread(MpwThreadHandle,
			 ThreadPriority,
			 &Priority,
			 sizeof(Priority));
  
  return(STATUS_SUCCESS);
}
