/*
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

VOID PsDumpThreads(VOID);

/* FUNCTIONS *****************************************************************/

BOOLEAN KeDeregisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
   UNIMPLEMENTED;
}

VOID KeInitializeBugCheck(VOID)
{
   InitializeListHead(&BugcheckCallbackListHead);
}

VOID KeInitializeCallbackRecord(PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
   UNIMPLEMENTED;
}

BOOLEAN KeRegisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
				   PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
				   PVOID Buffer,
				   ULONG Length,
				   PUCHAR Component)
{
   InsertTailList(&BugcheckCallbackListHead,&CallbackRecord->Entry);
   CallbackRecord->Length=Length;
   CallbackRecord->Buffer=Buffer;
   CallbackRecord->Component=Component;
   CallbackRecord->CallbackRoutine=CallbackRoutine;
   return(TRUE);
}

VOID KeBugCheckEx(ULONG BugCheckCode,
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
   __asm__("cli\n\t");      //PJS: disable interrupts first, then do the rest
   DbgPrint("Bug detected (code %x param %x %x %x %x)\n",BugCheckCode,
	  BugCheckParameter1,BugCheckParameter2,BugCheckParameter3,
	  BugCheckParameter4);
   PsDumpThreads();
   KeDumpStackFrames(0,64);
   
   for(;;)
	   __asm__("hlt\n\t");	//PJS: use HLT instruction, rather than busy wait
}

VOID KeBugCheck(ULONG BugCheckCode)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
{
   KeBugCheckEx(BugCheckCode,0,0,0,0);
}

