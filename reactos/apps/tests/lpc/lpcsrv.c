/* $Id: lpcsrv.c,v 1.10 2002/09/07 15:11:55 chorns Exp $
 *
 * DESCRIPTION: Simple LPC Server
 * PROGRAMMER:  David Welch
 */
#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lpctest.h"

static const char * MyName = "LPC-SRV";

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
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   HANDLE NamedPortHandle;
   HANDLE PortHandle;
   LPC_MAX_MESSAGE ConnectMsg;
   
   printf("%s: Lpc test server\n", MyName);

   InitializeObjectAttributes(&ObjectAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   
   printf("%s: Creating port \"%s\"...\n", MyName, TEST_PORT_NAME);
   Status = NtCreatePort(&NamedPortHandle,
			 &ObjectAttributes,
			 0,
			 0,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("%s: NtCreatePort() failed with status = 0x%08lX.\n", MyName, Status);
	return EXIT_FAILURE;
     }
   printf("%s: Port \"%s\" created (0x%x).\n\n", MyName, TEST_PORT_NAME, NamedPortHandle);
   
   for (;;)
   { 
     printf("%s: Listening for connections requests on port 0x%x...\n", MyName, NamedPortHandle);
     Status = NtListenPort(NamedPortHandle,
			 &ConnectMsg.Header);
     if (!NT_SUCCESS(Status))
       {
         printf("%s: NtListenPort() failed with status = 0x%08lX.\n", MyName, Status);
         return EXIT_FAILURE;
       }

     printf("%s: Received connection request 0x%08x on port 0x%x.\n", MyName,
        ConnectMsg.Header.MessageId, NamedPortHandle);
     printf("%s: Request from: PID=%x, TID=%x.\n", MyName,
        ConnectMsg.Header.ClientId.UniqueProcess, ConnectMsg.Header.ClientId.UniqueThread);
   
     printf("%s: Accepting connection request 0x%08x...\n", MyName, 
        ConnectMsg.Header.MessageId);
     Status = NtAcceptConnectPort(&PortHandle,
				NamedPortHandle,
				& ConnectMsg.Header,
				TRUE,
				0,
				NULL);
     if (!NT_SUCCESS(Status))
       {
         printf("%s: NtAcceptConnectPort() failed with status = 0x%08lX.\n", MyName, Status);
         return EXIT_FAILURE;
       }   
     printf("%s: Connection request 0x%08x accepted as port 0x%x.\n", MyName, 
        ConnectMsg.Header.MessageId, PortHandle);
   
     printf("%s: Completing connection for port 0x%x (0x%08x).\n", MyName, 
        PortHandle, ConnectMsg.Header.MessageId);
     Status = NtCompleteConnectPort(PortHandle);
     if (!NT_SUCCESS(Status))
       {
         printf("%s: NtCompleteConnectPort() failed with status = 0x%08lX.\n", MyName, Status);
         return EXIT_FAILURE;
       }
  
     printf("%s: Entering server loop for port 0x%x...\n", MyName, PortHandle); 
     for(;;)
       {
         LPC_MAX_MESSAGE Request;
	
         Status = NtReplyWaitReceivePort(PortHandle,
					0,
					NULL,
					&Request.Header);
	 if (!NT_SUCCESS(Status))
	   {
	     printf("%s: NtReplyWaitReceivePort() failed with status = 0x%08lX.\n", MyName, Status);
             return EXIT_FAILURE;
           }

         if (LPC_DATAGRAM == PORT_MESSAGE_TYPE(Request))
           {
             printf("%s: Datagram message contents are <%s>.\n",
               MyName, 
	       Request.Data);
           }
         else
           {
             printf("%s: Message with type %d received on port 0x%x.\n", MyName,
               PORT_MESSAGE_TYPE(Request), PortHandle);
             NtClose(PortHandle);
             printf("%s: Connected port 0x%x closed.\n\n", MyName, PortHandle);
             break;
           }
       }
   }
   return EXIT_SUCCESS;
}


/* EOF */
