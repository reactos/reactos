/* $Id: message.c,v 1.5 2002/09/08 10:23:06 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Message table functions
 * FILE:              lib/ntdll/rtl/message.c
 * PROGRAMER:         Eric Kohl <ekohl@zr-online.de>
 * REVISION HISTORY:
 *                    29/05/2001: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
RtlFindMessage(PVOID BaseAddress,
	       ULONG Type,
	       ULONG Language,
	       ULONG MessageId,
	       PRTL_MESSAGE_RESOURCE_ENTRY *MessageResourceEntry)
{
   LDR_RESOURCE_INFO ResourceInfo;
   PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
   PRTL_MESSAGE_RESOURCE_DATA MessageTable;
   NTSTATUS Status;
   ULONG EntryOffset, IdOffset;
   PRTL_MESSAGE_RESOURCE_ENTRY MessageEntry;
   ULONG i;

   DPRINT("RtlFindMessage()\n");

   ResourceInfo.Type = Type;
   ResourceInfo.Name = 1;
   ResourceInfo.Language = Language;

   Status = LdrFindResource_U(BaseAddress,
			      &ResourceInfo,
			      RESOURCE_DATA_LEVEL,
			      &ResourceDataEntry);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DPRINT("ResourceDataEntry: %p\n", ResourceDataEntry);

   Status = LdrAccessResource(BaseAddress,
			      ResourceDataEntry,
			      (PVOID*)&MessageTable,
			      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DPRINT("MessageTable: %p\n", MessageTable);

   DPRINT("NumberOfBlocks %lu\n", MessageTable->NumberOfBlocks);
   for (i = 0; i < MessageTable->NumberOfBlocks; i++)
     {
	DPRINT("LoId 0x%08lx  HiId 0x%08lx  Offset 0x%08lx\n",
	       MessageTable->Blocks[i].LowId,
	       MessageTable->Blocks[i].HighId,
	       MessageTable->Blocks[i].OffsetToEntries);
     }

   for (i = 0; i < MessageTable->NumberOfBlocks; i++)
     {
	if ((MessageId >= MessageTable->Blocks[i].LowId) &&
	    (MessageId <= MessageTable->Blocks[i].HighId))
	  {
	     EntryOffset = MessageTable->Blocks[i].OffsetToEntries;
	     IdOffset = MessageId - MessageTable->Blocks[i].LowId;
	     break;
	  }

	if (MessageId < MessageTable->Blocks[i].LowId)
	  {
	     return STATUS_MESSAGE_NOT_FOUND;
	  }
     }

   MessageEntry = (PRTL_MESSAGE_RESOURCE_ENTRY)((PUCHAR)MessageTable + MessageTable->Blocks[i].OffsetToEntries);

   DPRINT("EntryOffset 0x%08lx\n", EntryOffset);
   DPRINT("IdOffset 0x%08lx\n", IdOffset);

   DPRINT("MessageEntry: %p\n", MessageEntry);
   for (i = 0; i < IdOffset; i++)
     {
	MessageEntry = (PRTL_MESSAGE_RESOURCE_ENTRY)((PUCHAR)MessageEntry + (ULONG)MessageEntry->Length);
     }

   if (MessageEntry->Flags == 0)
     {
	DPRINT("AnsiText: %s\n", MessageEntry->Text);
     }
   else
     {
	DPRINT("UnicodeText: %S\n", (PWSTR)MessageEntry->Text);
     }

   if (MessageResourceEntry != NULL);
     {
	*MessageResourceEntry = MessageEntry;
     }

   return(STATUS_SUCCESS);
}


/*
RtlFormatMessage
 */

/* EOF */
