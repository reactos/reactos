/* $Id: lpcclt.c,v 1.5 2000/01/22 22:22:48 ea Exp $
 *
 * DESCRIPTION: Simple LPC Client
 * PROGRAMMER:  David Welch
 */
#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


int main(int argc, char* argv[])
{
   UNICODE_STRING PortName;
   NTSTATUS Status;
   HANDLE PortHandle;
   LPCMESSAGE Request;
   ULONG ConnectInfoLength;
   
   printf("(lpcclt.exe) Lpc client\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   
   printf("(lpcclt.exe) Connecting to port \"\\TestPort\"\n");
   ConnectInfoLength = 0;
   Status = NtConnectPort(&PortHandle,
			  &PortName,
			  NULL,
			  0,
			  0,
			  0,
			  NULL,
			  &ConnectInfoLength);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcclt.exe) Failed to connect (Status = 0x%08X)\n", Status);
	return EXIT_FAILURE;
     }
   
   strcpy(Request.MessageData, GetCommandLineA());
   Request.ActualMessageLength = strlen(Request.MessageData);
   Request.TotalMessageLength = sizeof(LPCMESSAGE);
   
   printf("(lpcclt.exe) Sending message \"%s\"\n", (char *) Request.MessageData);
   Status = NtRequestPort(PortHandle, &Request);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcclt.exe) Failed to send request (Status = 0x%=8X)\n", Status);
	return EXIT_FAILURE;
     }
   
   printf("(lpcclt.exe) Succeeded\n");
   return EXIT_SUCCESS;
}
