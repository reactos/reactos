#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

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


void main(int argc, char* argv[])
{
   UNICODE_STRING PortName;
   NTSTATUS Status;
   HANDLE PortHandle;
   LPC_MESSAGE Request;
   char buffer[255];
   
   printf("(lpcclt.exe) Lpc client\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   
   printf("(lpcclt.exe) Connecting to port\n");
   Status = NtConnectPort(&PortHandle,
			  &PortName,
			  NULL,
			  0,
			  0,
			  0,
			  0,
			  0);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcclt.exe) Failed to connect\n");
	return;
     }
   
   strcpy(buffer, GetCommandLineA());
   Request.Buffer = buffer;
   Request.Length = strlen(buffer);
   
   printf("(lpcclt.exe) Sending message\n");
   Status = NtRequestWaitReplyPort(PortHandle,
				   NULL,
				   &Request);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcclt.exe) Failed to send request\n");
	return;
     }
   
   printf("(lpcclt.exe) Succeeded\n");
}
