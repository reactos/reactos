#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

VOID InstanceThread (LPVOID);

VOID GetAnswerToRequest (LPTSTR, LPTSTR, LPDWORD)
{
}

int xx = 0;

DWORD main (VOID)
{
   BOOL fConnected;
   DWORD dwThreadId;
   HANDLE hPipe, hThread;
   LPTSTR lpszPipename = "\\\\.\\pipe\\mynamedpipe";

   for (;;)
     {
	hPipe = CreateNamedPipe (lpszPipename,	
				 PIPE_ACCESS_DUPLEX,	
				 PIPE_TYPE_MESSAGE |	
				 PIPE_READMODE_MESSAGE |	
				 PIPE_WAIT,	
				 PIPE_UNLIMITED_INSTANCES,	
				 BUFSIZE,	
				 BUFSIZE,	
				 PIPE_TIMEOUT,	
				 NULL);	
	if (hPipe == INVALID_HANDLE_VALUE)
	  MyErrExit ("CreatePipe");
	
	fConnected = ConnectNamedPipe (hPipe,
				       NULL) ? TRUE : (GetLastError () ==
					    ERROR_PIPE_CONNECTED);
	if (fConnected)
	  {			
	     hThread = CreateThread (NULL,	
				     0,	
				     (LPTHREAD_START_ROUTINE) InstanceThread,
				     (LPVOID) hPipe,	
				     0,	
				     &dwThreadId);	
	     if (hThread == NULL)
	       MyErrExit ("CreateThread");
	  }	
	else		
	  {
	     CloseHandle (hPipe);
	  }
     }
   return 1;
}

VOID InstanceThread (LPVOID lpvParam)
{
   CHAR chRequest[BUFSIZE];
   CHAR chReply[BUFSIZE];
   DWORD cbBytesRead, cbReplyBytes, cbWritten;
   BOOL fSuccess;
   HANDLE hPipe;
   
   hPipe = (HANDLE) lpvParam;
   while (1)     
     {      
	fSuccess = ReadFile (hPipe,	
			     chRequest,	
			     BUFSIZE,	
			     &cbBytesRead,	
			     NULL);	
	if (!fSuccess || cbBytesRead == 0)
	  break;
	GetAnswerToRequest (chRequest, chReply, &cbReplyBytes);
	
	fSuccess = WriteFile (hPipe,	
			      chReply,	
			      cbReplyBytes,	
			      &cbWritten,	
			      NULL);	
      if (!fSuccess || cbReplyBytes != cbWritten)
	break;
    }

   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe (hPipe);
   CloseHandle (hPipe);
}
