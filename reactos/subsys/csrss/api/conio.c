/* $Id: conio.c,v 1.3 2000/03/22 18:35:59 dwelch Exp $
 *
 * reactos/subsys/csrss/api/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"

/* GLOBALS *******************************************************************/

static HANDLE ConsoleDeviceHandle;
static HANDLE KeyboardDeviceHandle;

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PLPCMESSAGE* LpcReply)
{
   PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);

   Reply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PLPCMESSAGE* LpcReply)
{
   PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);

   Reply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PLPCMESSAGE* LpcReply)
{
   KEY_EVENT_RECORD KeyEventRecord;
   BOOL  stat = TRUE;
   PCHAR Buffer;
   DWORD Result;
   int   i;
   ULONG nNumberOfCharsToRead;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   PCSRSS_API_REPLY Reply;
   
//   DbgPrint("CSR: CsrReadConsole()\n");
   
   nNumberOfCharsToRead = 
     LpcMessage->Data.ReadConsoleRequest.NrCharactersToRead;
   
//   DbgPrint("CSR: NrCharactersToRead %d\n", nNumberOfCharsToRead);
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY) + 
     nNumberOfCharsToRead;
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);   
   Buffer = Reply->Data.ReadConsoleReply.Buffer;
   
   Status = STATUS_SUCCESS;
   
   for (i=0; (NT_SUCCESS(Status) && i<nNumberOfCharsToRead);)     
     {
//	DbgPrint("CSR: Doing read file (KeyboardDeviceHandle %x)\n",
//		 KeyboardDeviceHandle);
	Status = NtReadFile(KeyboardDeviceHandle,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    &KeyEventRecord,
			    sizeof(KEY_EVENT_RECORD),
			    NULL,
			    0);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("CSR: Read failed, bailing\n");
	  }
        if (KeyEventRecord.bKeyDown && KeyEventRecord.uChar.AsciiChar != 0)
	  {
	     Buffer[i] = KeyEventRecord.uChar.AsciiChar;
	     //DbgPrint("CSR: Read '%c'\n", Buffer[i]);
	     i++;
	  }
     }
   Reply->Data.ReadConsoleReply.NrCharactersRead = i;
   Reply->Status = Status;
   return(Status);
}


NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST Message,
			 PLPCMESSAGE* LpcReply)
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);

//   DbgPrint("CSR: CsrWriteConsole()\n");
//   DbgPrint("CSR: ConsoleDeviceHandle %x\n", ConsoleDeviceHandle);
//   DbgPrint("CSR: NrCharactersToWrite %d\n",
//	    Message->Data.WriteConsoleRequest.NrCharactersToWrite);
//   DbgPrint("CSR: Buffer %s\n",
//	    Message->Data.WriteConsoleRequest.Buffer);
   
   Status = NtWriteFile(ConsoleDeviceHandle,
			NULL,
			NULL,
			NULL,
			&Iosb,
			Message->Data.WriteConsoleRequest.Buffer,
			Message->Data.WriteConsoleRequest.NrCharactersToWrite,
			NULL,
			0);
   if (!NT_SUCCESS(Status))
     {
//	DbgPrint("CSR: Write failed\n");
	return(Status);
     }
   Reply->Data.WriteConsoleReply.NrCharactersWritten = Iosb.Information;
   Reply->Status = STATUS_SUCCESS;
   return(STATUS_SUCCESS);
}

VOID CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console)
{
}

VOID CsrInitConsoleSupport(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   
   DbgPrint("CSR: CsrInitConsoleSupport()\n");
   
   RtlInitUnicodeString(&DeviceName, L"\\??\\BlueScreen");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&ConsoleDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open console. Expect problems.\n");
     }
//   DbgPrint("CSR: ConsoleDeviceHandle %x\n", ConsoleDeviceHandle);
   
   RtlInitUnicodeString(&DeviceName, L"\\??\\Keyboard");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&KeyboardDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open keyboard. Expect problems.\n");
	for(;;);
     }
   
   DbgPrint("CSR: KeyboardDeviceHandle %x\n", KeyboardDeviceHandle);
}

/* EOF */
