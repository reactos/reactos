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
   
   printf("Lpc client\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   
   printf("Connecting to port\n");
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
	printf("Failed to connect\n");
	return;
     }
   
   strcpy(buffer, GetCommandLineA());
   Request.Buffer = buffer;
	
   Status = NtRequestWaitReplyPort(PortHandle,
				   NULL,
				   &Request);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to send request\n");
	return;
     }
   
   printf("Succeeded\n");
}
