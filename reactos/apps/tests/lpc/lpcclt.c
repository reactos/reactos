/* $Id: lpcclt.c,v 1.11 2002/09/08 10:21:58 chorns Exp $
 *
 * DESCRIPTION: Simple LPC Client
 * PROGRAMMER:  David Welch
 */
#include <ddk/ntddk.h>
#include <windows.h>
#include <napi/lpc.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lpctest.h"

const char * MyName = "LPC-CLI";
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
   UNICODE_STRING PortName = UNICODE_STRING_INITIALIZER(TEST_PORT_NAME_U);
   NTSTATUS Status;
   HANDLE PortHandle;
   LPC_MAX_MESSAGE Request;
   ULONG ConnectInfo;
   ULONG ConnectInfoLength = 0;
   SECURITY_QUALITY_OF_SERVICE Sqos;
   
   printf("%s: Lpc test client\n", MyName);
   
   printf("%s: Connecting to port \"%s\"...\n", MyName, TEST_PORT_NAME);
   ConnectInfoLength = 0;
   ZeroMemory (& Sqos, sizeof Sqos);
   Status = NtConnectPort(&PortHandle,
			  &PortName,
			  & Sqos,
			  0,
			  0,
			  0,
			  NULL,
			  &ConnectInfoLength);
   if (!NT_SUCCESS(Status))
     {
	printf("%s: NtConnectPort() failed with status = 0x%08X.\n", MyName, Status);
	return EXIT_FAILURE;
     }

   printf("%s: Connected to \"%s\" with anonymous port 0x%x.\n", MyName, TEST_PORT_NAME, PortHandle);

   ZeroMemory(& Request, sizeof Request);
   strcpy(Request.Data, GetCommandLineA());
   Request.Header.DataSize = strlen(Request.Data);
   Request.Header.MessageSize = sizeof(LPC_MESSAGE_HEADER) + 
     Request.Header.DataSize;
   
   printf("%s: Sending to port 0x%x message \"%s\"...\n", 
          MyName,
          PortHandle,
	  (char *) Request.Data);
   Status = NtRequestPort(PortHandle, 
			  &Request.Header);
   if (!NT_SUCCESS(Status))
     {
	printf("%s: NtRequestPort(0x%x) failed with status = 0x%8X.\n", 
               MyName,
               PortHandle,
	       Status);
	return EXIT_FAILURE;
     }
   
   printf("%s: Sending datagram to port 0x%x succeeded.\n", MyName, PortHandle);

   Sleep(2000);

   printf("%s: Disconnecting...", MyName);
   NtClose (PortHandle);

   return EXIT_SUCCESS;
}
