/* $Id: conio.c,v 1.4 2000/04/03 21:54:41 dwelch Exp $
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
			 PCSRSS_API_REPLY LpcReply)
{
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   LpcReply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY LpcReply)
{
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   LpcReply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY LpcReply)
{
   KEY_EVENT_RECORD KeyEventRecord;
   PCHAR Buffer;
   int   i;
   ULONG nNumberOfCharsToRead;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   
//   DbgPrint("CSR: CsrReadConsole()\n");
   
   nNumberOfCharsToRead = 
     LpcMessage->Data.ReadConsoleRequest.NrCharactersToRead;
   
//   DbgPrint("CSR: NrCharactersToRead %d\n", nNumberOfCharsToRead);
   
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY) + 
     nNumberOfCharsToRead;
   LpcReply->Header.DataSize = LpcReply->Header.MessageSize -
     sizeof(LPC_MESSAGE_HEADER);
   Buffer = LpcReply->Data.ReadConsoleReply.Buffer;
   
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
//	     DbgPrint("CSR: Read failed, bailing\n");
	  }
        if (KeyEventRecord.bKeyDown && KeyEventRecord.uChar.AsciiChar != 0)
	  {
	     Buffer[i] = KeyEventRecord.uChar.AsciiChar;
//	     DbgPrint("CSR: Read '%c'\n", Buffer[i]);
	     i++;
	  }
     }
   LpcReply->Data.ReadConsoleReply.NrCharactersRead = i;
   LpcReply->Status = Status;
   return(Status);
}


NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST Message,
			 PCSRSS_API_REPLY LpcReply)
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

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
   LpcReply->Data.WriteConsoleReply.NrCharactersWritten = Iosb.Information;
   LpcReply->Status = STATUS_SUCCESS;
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
     }
   
   DbgPrint("CSR: KeyboardDeviceHandle %x\n", KeyboardDeviceHandle);
}

/* EOF */
