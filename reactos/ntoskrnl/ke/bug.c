/* $Id: bug.c,v 1.13 2000/07/02 10:49:30 ekohl Exp $
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

BOOLEAN
STDCALL
KeDeregisterBugCheckCallback (
	PKBUGCHECK_CALLBACK_RECORD	CallbackRecord
	)
{
   UNIMPLEMENTED;
}

BOOLEAN
STDCALL
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

VOID
STDCALL
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
   PsDumpThreads();
   KeDumpStackFrames(&((&BugCheckCode)[-1]),64);
   
   for(;;)
	   __asm__("hlt\n\t");	//PJS: use HLT instruction, rather than busy wait
}

VOID
STDCALL
KeBugCheck (
	ULONG	BugCheckCode
	)
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
   PsDumpThreads();
   KeDumpStackFrames(&((&BugCheckCode)[-1]), 80);
   for(;;)
     {
	__asm__("hlt\n\t");
     }
}

/* EOF */
