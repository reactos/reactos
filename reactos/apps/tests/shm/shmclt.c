#include <ddk/ntddk.h>
#include <windows.h>
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


int
main(int argc, char* argv[])
{
   HANDLE Section;
   PVOID BaseAddress;
   char buffer[256];
   
   printf("Shm test server\n");
   
   Section = OpenFileMappingW (
//		PAGE_EXECUTE_READWRITE, invalid parameter
		FILE_MAP_WRITE,
		FALSE,
		L"TestSection"
		);
   if (Section == NULL)
     {
	printf("Failed to open section (err=%d)", GetLastError());
	return 1;
     }
   
   BaseAddress = MapViewOfFile(Section,
			       FILE_MAP_ALL_ACCESS,
			       0,
			       0,
			       8192);
   if (BaseAddress == NULL)
     {
	printf("Failed to map section (err=%d)\n", GetLastError());
	return 1;
     }
   printf("BaseAddress %x\n", (UINT) BaseAddress);
   printf("Copying from section\n");
   strcpy(buffer, BaseAddress);
   printf("Copyed <%s>\n", buffer);
   
//   for(;;);
	return 0;
}

