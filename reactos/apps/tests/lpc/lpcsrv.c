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
   
   printf("Lpc test server\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   
   printf("Creating port\n");
   Status = NtCreatePort(&NamedPortHandle,
			 0,
			 &ObjectAttributes,
			 0,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to create port\n");
	return;
     }
   
   
   printf("Listening for connections\n");
   Status = NtListenPort(NamedPortHandle,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to listen for connections\n");
	return;
     }
   
   printf("Accepting connections\n");
   Status = NtAcceptConnectPort(NamedPortHandle,
				&PortHandle,
				0,
				0,
				0,
				0);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to accept connection\n");
	return;
     }   
   
   printf("Completing connection\n");
   Status = NtCompleteConnectPort(PortHandle);
   if (!NT_SUCCESS(Status))
     {
	printf("Failed to complete connection\n");
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
	     printf("Failed to receive request\n");
	     return;
	  }
	
	printf("Message contents are <%s>\n", Request.Buffer);
     }
}
