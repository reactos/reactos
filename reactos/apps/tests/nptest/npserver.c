#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>

#define BUFSIZE 1024
#define PIPE_TIMEOUT 1000

VOID InstanceThread (LPVOID);

VOID
GetAnswerToRequest(LPTSTR lpRequest,
		   LPTSTR lpReply,
		   LPDWORD lpcbReplyBytes)
{
}

VOID MyErrExit(LPTSTR Message)
{
//	MessageBox(NULL, Message, NULL, MB_OK);
   puts(Message);
   ExitProcess(0);
}




int xx = 0;

int main(int argc, char *argv[])
{
   BOOL fConnected;
   DWORD dwThreadId;
   HANDLE hPipe, hThread;
   LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe");

//   for (;;)
//     {
	hPipe = CreateNamedPipe(lpszPipename,
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
	  {
	     printf("CreateNamedPipe() failed\n");
	     return 0;
	  }

	fConnected = ConnectNamedPipe(hPipe,
				      NULL) ? TRUE : (GetLastError () ==
					    ERROR_PIPE_CONNECTED);
	if (fConnected)
	  {
	     printf("Pipe connected!\n");

	     DisconnectNamedPipe(hPipe);

#if 0
	     hThread = CreateThread(NULL,
				    0,
				    (LPTHREAD_START_ROUTINE) InstanceThread,
				    (LPVOID) hPipe,
				    0,
				    &dwThreadId);
	     if (hThread == NULL)
	       MyErrExit("CreateThread");
#endif
	  }
	else
	  {
//	     CloseHandle(hPipe);
	  }
//     }

   CloseHandle(hPipe);

   return 0;
}

VOID InstanceThread (LPVOID lpvParam)
{
   CHAR chRequest[BUFSIZE];
   CHAR chReply[BUFSIZE];
   DWORD cbBytesRead, cbReplyBytes, cbWritten;
   BOOL fSuccess;
   HANDLE hPipe;
   
   hPipe = (HANDLE)lpvParam;
   while (1)
     {
	fSuccess = ReadFile(hPipe,
			    chRequest,
			    BUFSIZE,
			    &cbBytesRead,
			    NULL);
	if (!fSuccess || cbBytesRead == 0)
	  break;

	GetAnswerToRequest(chRequest, chReply, &cbReplyBytes);

	fSuccess = WriteFile(hPipe,
			     chReply,
			     cbReplyBytes,
			     &cbWritten,
			     NULL);
	if (!fSuccess || cbReplyBytes != cbWritten)
	  break;
    }

   FlushFileBuffers(hPipe);
   DisconnectNamedPipe(hPipe);
   CloseHandle(hPipe);
}
