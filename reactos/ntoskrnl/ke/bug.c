/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static LIST_ENTRY BugcheckCallbackListHead = {NULL,NULL};

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
   DbgPrint("Bug detected (code %x param %x %x %x %x)\n",BugCheckCode,
	  BugCheckParameter1,BugCheckParameter2,BugCheckParameter3,
	  BugCheckParameter4);
   for(;;);
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

