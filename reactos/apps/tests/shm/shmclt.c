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
   HANDLE Section;
   PVOID BaseAddress;
   char buffer[256];
   
   printf("Shm test server\n");
   
   Section = OpenFileMappingW(PAGE_EXECUTE_READWRITE,
			      FALSE,
			      L"\\TestSection");
   if (Section == NULL)
     {
	printf("Failed to open section");
	return;
     }
   
   BaseAddress = MapViewOfFile(Section,
			       FILE_MAP_ALL_ACCESS,
			       0,
			       0,
			       8192);
   printf("BaseAddress %x\n", BaseAddress);
   if (BaseAddress == NULL)
     {
	printf("Failed to map section\n");
     }
   printf("Copying from section\n");
   strcpy(buffer, BaseAddress);
   printf("Copyed <%s>\n", buffer);
   
//   for(;;);
}

