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
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   HANDLE NamedPortHandle;
   HANDLE PortHandle;
   
   printf("(lpcsrv.exe) Lpc test server\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   
   printf("(lpcsrv.exe) Creating port\n");
   Status = NtCreatePort(&NamedPortHandle,
			 0,
			 &ObjectAttributes,
			 0,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to create port\n");
	return;
     }
   
   
   printf("(lpcsrv.exe) Listening for connections\n");
   Status = NtListenPort(NamedPortHandle,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to listen for connections\n");
	return;
     }
   
   printf("(lpcsrv.exe) Accepting connections\n");
   Status = NtAcceptConnectPort(NamedPortHandle,
				&PortHandle,
				0,
				0,
				0,
				0);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to accept connection\n");
	return;
     }   
   
   printf("(lpcsrv.exe) Completing connection\n");
   Status = NtCompleteConnectPort(PortHandle);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to complete connection\n");
	return;
     }
   
   for(;;)
     {
	LPC_MESSAGE Request;
	char buffer[255];
	
	Request.Buffer = buffer;
	
	Status = NtRequestWaitReplyPort(PortHandle,
					&Request,
					NULL);
	if (!NT_SUCCESS(Status))
	  {
	     printf("(lpcsrv.exe) Failed to receive request\n");
	     return;
	  }
	
	printf("(lpcsrv.exe) Message contents are <%s>\n", Request.Buffer);
     }
}
