#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>

#define BUFSIZE 1024
#define MAILSLOT_TIMEOUT 1000


int main(int argc, char *argv[])
{
   HANDLE hMailslot;
   LPSTR lpszMailslotName = "\\\\.\\MAILSLOT\\mymailslot";
   LPSTR lpszTestMessage = "Mailslot test message!";
   DWORD cbLength, cbWritten;
   
   hMailslot = CreateFile(lpszMailslotName,
			  GENERIC_WRITE,
			  FILE_SHARE_READ,
			  (LPSECURITY_ATTRIBUTES)NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  (HANDLE)NULL);
   printf("hMailslot %x\n", (DWORD)hMailslot);
   if (hMailslot == INVALID_HANDLE_VALUE)
     {
	printf("CreateFile() failed\n");
	return 0;
     }
   
   cbLength = (ULONG)strlen(lpszTestMessage)+1;
   
   WriteFile(hMailslot,
	     lpszTestMessage,
	     cbLength,
	     &cbWritten,
	     NULL);
   
   CloseHandle(hMailslot);
   
   return 0;
}

/* EOF */

