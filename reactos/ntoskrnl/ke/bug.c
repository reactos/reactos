/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: bug.c,v 1.32 2003/07/11 01:23:15 royce Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Unchecked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Phillip Susi: 12/8/99: Minor fix
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/kd.h>
#include <internal/ke.h>
#include <internal/ps.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static LIST_ENTRY BugcheckCallbackListHead = {NULL,NULL};
static ULONG InBugCheck;

VOID PsDumpThreads(VOID);

/* FUNCTIONS *****************************************************************/

VOID
KeInitializeBugCheck(VOID)
{
  InitializeListHead(&BugcheckCallbackListHead);
  InBugCheck = 0;
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
KeDeregisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeRegisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
			   PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
			   PVOID Buffer,
			   ULONG Length,
			   PUCHAR Component)
{
  InsertTailList(&BugcheckCallbackListHead, &CallbackRecord->Entry);
  CallbackRecord->Length = Length;
  CallbackRecord->Buffer = Buffer;
  CallbackRecord->Component = Component;
  CallbackRecord->CallbackRoutine = CallbackRoutine;
  return(TRUE);
}

VOID STDCALL
KeBugCheckWithTf(ULONG BugCheckCode, 	     
		 ULONG BugCheckParameter1,
		 ULONG BugCheckParameter2,
		 ULONG BugCheckParameter3,
		 ULONG BugCheckParameter4,
		 PKTRAP_FRAME Tf)
{
  PRTL_MESSAGE_RESOURCE_ENTRY Message;
  NTSTATUS Status;

  /* Make sure we're switching back to the blue screen and print messages on it */
  HalReleaseDisplayOwnership();  
  KdDebugState |= KD_DEBUG_SCREEN;

  __asm__("cli\n\t");
  DbgPrint("Bug detected (code %x param %x %x %x %x)\n",
	   BugCheckCode,
	   BugCheckParameter1,
	   BugCheckParameter2,
	   BugCheckParameter3,
	   BugCheckParameter4);

  Status = RtlFindMessage((PVOID)KERNEL_BASE, //0xC0000000,
			  11, //RT_MESSAGETABLE,
			  0x09, //0x409,
			  BugCheckCode,
			  &Message);
  if (NT_SUCCESS(Status))
    {
      if (Message->Flags == 0)
	DbgPrint("  %s\n", Message->Text);
      else
	DbgPrint("  %S\n", (PWSTR)Message->Text);
    }
  else
    {
      DbgPrint("  No message text found!\n\n");
    }

  if (InBugCheck == 1)
    {
      DbgPrint("Recursive bug check halting now\n");
      for (;;)
	{
	  __asm__ ("hlt\n\t");
	}
    }
  InBugCheck = 1;
  KiDumpTrapFrame(Tf, BugCheckParameter1, BugCheckParameter2);
  MmDumpToPagingFile(BugCheckCode, BugCheckParameter1, 
		     BugCheckParameter2, BugCheckParameter3,
		     BugCheckParameter4, Tf);

  if (KdDebuggerEnabled)
    {
      __asm__("sti\n\t");
      DbgBreakPoint();
      __asm__("cli\n\t");
    }

  for (;;)
    {
      __asm__("hlt\n\t");
    }
}

/*
 * @implemented
 */
VOID STDCALL
KeBugCheckEx(ULONG BugCheckCode,
	     ULONG BugCheckParameter1,
	     ULONG BugCheckParameter2,
	     ULONG BugCheckParameter3,
	     ULONG BugCheckParameter4)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
{
  PRTL_MESSAGE_RESOURCE_ENTRY Message;
  NTSTATUS Status;
  
  /* Make sure we're switching back to the blue screen and print messages on it */
  HalReleaseDisplayOwnership();  
  KdDebugState |= KD_DEBUG_SCREEN;

  __asm__("cli\n\t");
  DbgPrint("Bug detected (code %x param %x %x %x %x)\n",
	   BugCheckCode,
	   BugCheckParameter1,
	   BugCheckParameter2,
	   BugCheckParameter3,
	   BugCheckParameter4);

  Status = RtlFindMessage((PVOID)KERNEL_BASE, //0xC0000000,
			  11, //RT_MESSAGETABLE,
			  0x09, //0x409,
			  BugCheckCode,
			  &Message);
  if (NT_SUCCESS(Status))
    {
      if (Message->Flags == 0)
	DbgPrint("  %s\n", Message->Text);
      else
	DbgPrint("  %S\n", (PWSTR)Message->Text);
    }
  else
    {
      DbgPrint("  No message text found!\n\n");
    }

  if (InBugCheck == 1)
    {
      DbgPrint("Recursive bug check halting now\n");
      for (;;)
	{
	  __asm__("hlt\n\t");
	}
    }
  InBugCheck = 1;
  if (PsGetCurrentProcess() != NULL)
    {
      DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
      DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
    }
  if (PsGetCurrentThread() != NULL)
    {
      DbgPrint("Thrd: %x Tid: %x\n",
	       PsGetCurrentThread(),
	       PsGetCurrentThread()->Cid.UniqueThread);
    }
  KeDumpStackFrames((PULONG)__builtin_frame_address(0));
  MmDumpToPagingFile(BugCheckCode, BugCheckParameter1, 
		     BugCheckParameter2, BugCheckParameter3,
		     BugCheckParameter4, NULL);

  if (KdDebuggerEnabled)
    {
      __asm__("sti\n\t");
      DbgBreakPoint();
      __asm__("cli\n\t");
    }

  for (;;)
    {
      __asm__("hlt\n\t");
    }
}

/*
 * @implemented
 */
VOID STDCALL
KeBugCheck(ULONG BugCheckCode)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
{
  KeBugCheckEx(BugCheckCode, 0, 0, 0, 0);
}

/* EOF */
