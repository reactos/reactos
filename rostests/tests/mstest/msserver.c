

#include <windows.h>


int main(int argc, char *argv[])
{
   HANDLE hMailslot;
   CHAR chBuf[512];
   BOOL fResult;
   DWORD cbRead;
   LPTSTR lpszMailslotName = "\\\\.\\mailslot\\mymailslot";

   hMailslot = CreateMailslot(lpszMailslotName,
			      512,
			      MAILSLOT_WAIT_FOREVER,
			      NULL);
for (;;)
{
   fResult = ReadFile(hMailslot,
		      chBuf,
		      512,
		      &cbRead,
		      NULL);
   if (fResult == FALSE)
     {
	printf("ReadFile() failed!\n");
	CloseHandle(hMailslot);
	return 0;
     }

   printf("Data read: %s\n", chBuf);
}

   CloseHandle(hMailslot);

   return 0;
}

/* EOF */
