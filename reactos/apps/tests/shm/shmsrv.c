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

VOID STDCALL ApcRoutine(PVOID Context,
			PIO_STATUS_BLOCK IoStatus,
			ULONG Reserved)
{
   printf("(apc.exe) ApcRoutine(Context %x)\n", Context);
}

void main(int argc, char* argv[])
{
   HANDLE Section;
   PVOID BaseAddress;
   
   printf("Shm test server\n");
   
   Section = CreateFileMappingW(NULL,
				NULL,
				PAGE_EXECUTE_READWRITE,
				0,
				8192,
				L"\\TestSection");
   if (Section == NULL)
     {
	printf("Failed to create section");
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
   
   printf("Copying to section\n");
   printf("Copying %s\n", GetCommandLineA());
   strcpy(BaseAddress, GetCommandLineA());
   
   for(;;);
}

