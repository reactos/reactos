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
/* $Id: bug.c,v 1.16 2001/03/14 00:21:22 dwelch Exp $
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

BOOLEAN STDCALL
KeDeregisterBugCheckCallback (PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
  UNIMPLEMENTED;
}

BOOLEAN STDCALL
KeRegisterBugCheckCallback (PKBUGCHECK_CALLBACK_RECORD	CallbackRecord,
			    PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
			    PVOID				Buffer,
			    ULONG				Length,
			    PUCHAR				Component)
{
  InsertTailList(&BugcheckCallbackListHead,&CallbackRecord->Entry);
  CallbackRecord->Length=Length;
  CallbackRecord->Buffer=Buffer;
  CallbackRecord->Component=Component;
  CallbackRecord->CallbackRoutine=CallbackRoutine;
  return(TRUE);
}

VOID STDCALL
KeBugCheckEx (ULONG	BugCheckCode,
	      ULONG	BugCheckParameter1,
	      ULONG	BugCheckParameter2,
	      ULONG	BugCheckParameter3,
	      ULONG	BugCheckParameter4)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
{
  /* PJS: disable interrupts first, then do the rest */
  __asm__("cli\n\t");      
   DbgPrint("Bug detected (code %x param %x %x %x %x)\n",BugCheckCode,
	  BugCheckParameter1,BugCheckParameter2,BugCheckParameter3,
	  BugCheckParameter4);
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
//   PsDumpThreads();
   KeDumpStackFrames(&((&BugCheckCode)[-1]),64);
   
#if 1
   for(;;)
     {
       /* PJS: use HLT instruction, rather than busy wait */
       __asm__("hlt\n\t");	
     }
#else
   for(;;);
#endif   
}

VOID STDCALL
KeBugCheck (ULONG	BugCheckCode)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
{
   __asm__("cli\n\t");
   DbgPrint("Bug detected (code %x)\n", BugCheckCode);
   if (InBugCheck == 1)
     {
	DbgPrint("Recursive bug check halting now\n");
	for(;;);
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
//   PsDumpThreads();
   KeDumpStackFrames(&((&BugCheckCode)[-1]), 80);
#if 1
   for(;;)
     {
	__asm__("hlt\n\t");
     }
#else
   for(;;);
#endif
}

/* EOF */
