/* $Id: shmsrv.c,v 1.2 1999/12/02 20:53:52 dwelch Exp $
 *
 * FILE  : reactos/apps/shm/shmsrv.c
 * AUTHOR: David Welch
 */
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
   printf("(apc.exe) ApcRoutine(Context %x)\n", (UINT) Context);
}


int
main(int argc, char* argv[])
{
   HANDLE Section;
   PVOID BaseAddress;
   
   printf("Shm test server\n");
   
   Section = CreateFileMappingW (
			(HANDLE) 0xFFFFFFFF,
			NULL,
//			PAGE_EXECUTE_READWRITE, invalid parameter
			PAGE_READWRITE, 
			0,
			8192,
			L"TestSection"
			);
   if (Section == NULL)
     {
	printf("Failed to create section (err=%d)", GetLastError());
	return 1;
     }
   
   BaseAddress = MapViewOfFile(Section,
			       FILE_MAP_ALL_ACCESS,
			       0,
			       0,
			       8192);
   printf("BaseAddress %x\n", (UINT) BaseAddress);
   if (BaseAddress == NULL)
     {
	printf("Failed to map section\n");
     }
   
   printf("Copying to section\n");
   printf("Copying %s\n", GetCommandLineA());
   strcpy(BaseAddress, GetCommandLineA());
   
   for(;;);

   return 0;
}

