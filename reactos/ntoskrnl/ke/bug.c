/* $Id: bug.c,v 1.15 2001/03/07 08:57:08 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@cwcom.net)
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

VOID KeInitializeBugCheck(VOID)
{
   InitializeListHead(&BugcheckCallbackListHead);
   InBugCheck = 0;
}

BOOLEAN STDCALL
KeDeregisterBugCheckCallback (
	PKBUGCHECK_CALLBACK_RECORD	CallbackRecord
	)
{
   UNIMPLEMENTED;
}

BOOLEAN STDCALL
KeRegisterBugCheckCallback (
	PKBUGCHECK_CALLBACK_RECORD	CallbackRecord,
	PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
	PVOID				Buffer,
	ULONG				Length,
	PUCHAR				Component
	)
{
   InsertTailList(&BugcheckCallbackListHead,&CallbackRecord->Entry);
   CallbackRecord->Length=Length;
   CallbackRecord->Buffer=Buffer;
   CallbackRecord->Component=Component;
   CallbackRecord->CallbackRoutine=CallbackRoutine;
   return(TRUE);
}

VOID STDCALL
KeBugCheckEx (
	ULONG	BugCheckCode,
	ULONG	BugCheckParameter1,
	ULONG	BugCheckParameter2,
	ULONG	BugCheckParameter3,
	ULONG	BugCheckParameter4
	)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
{
   __asm__("cli\n\t");      //PJS: disable interrupts first, then do the rest
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
