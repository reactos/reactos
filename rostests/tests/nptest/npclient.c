#include <windows.h>

VOID MyErrExit(LPTSTR Message)
{
//	MessageBox(NULL, Message, NULL, MB_OK);
   puts(Message);
   ExitProcess(0);
}

int main(int argc, char *argv[])
{
   HANDLE hPipe;
   LPVOID lpvMessage;
   CHAR chBuf[512];
   BOOL fSuccess;
   DWORD cbRead, cbWritten, dwMode;
   LPTSTR lpszPipename = "\\\\.\\pipe\\mynamedpipe";

// Try to open a named pipe; wait for it, if necessary.

   while (1)
   {
      hPipe = CreateFile(
         lpszPipename,   // pipe name
         GENERIC_READ |  // read and write access
         GENERIC_WRITE,
         0,              // no sharing
         NULL,           // no security attributes
         OPEN_EXISTING,  // opens existing pipe
         0,              // default attributes
         NULL);          // no template file

   // Break if the pipe handle is valid.

      if (hPipe != INVALID_HANDLE_VALUE)
         break;

      // Exit if an error other than ERROR_PIPE_BUSY occurs.

      if (GetLastError() != ERROR_PIPE_BUSY)
         MyErrExit("Could not open pipe");

      // All pipe instances are busy, so wait for 20 seconds.

      if (! WaitNamedPipe(lpszPipename, 20000) )
         MyErrExit("Could not open pipe");
   }

// The pipe connected; change to message-read mode.

   dwMode = PIPE_READMODE_MESSAGE;
   fSuccess = SetNamedPipeHandleState(
      hPipe,    // pipe handle
      &dwMode,  // new pipe mode
      NULL,     // don't set maximum bytes
      NULL);    // don't set maximum time
   if (!fSuccess)
      MyErrExit("SetNamedPipeHandleState");

// Send a message to the pipe server.

   lpvMessage = (argc > 1) ? argv[1] : "default message";

   fSuccess = WriteFile(
      hPipe,                  // pipe handle
      lpvMessage,             // message
      strlen(lpvMessage) + 1, // message length
      &cbWritten,             // bytes written
      NULL);                  // not overlapped
   if (! fSuccess)
      MyErrExit("WriteFile");

   do
   {
   // Read from the pipe.

      fSuccess = ReadFile(
         hPipe,    // pipe handle
         chBuf,    // buffer to receive reply
         512,      // size of buffer
         &cbRead,  // number of bytes read
         NULL);    // not overlapped

      if (! fSuccess && GetLastError() != ERROR_MORE_DATA)
         break;

      // Reply from the pipe is written to STDOUT.

      if (! WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
         chBuf, cbRead, &cbWritten, NULL))
      {
         break;
      }

   } while (! fSuccess);  // repeat loop if ERROR_MORE_DATA

   CloseHandle(hPipe);

   return 0;
}
