/* $Id: shmsrv.c,v 1.3 1999/12/30 01:51:36 dwelch Exp $
 *
 * FILE  : reactos/apps/shm/shmsrv.c
 * AUTHOR: David Welch
 */
#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
   HANDLE Section;
   PVOID BaseAddress;
   
   printf("Shm test server\n");
   
   Section = CreateFileMappingW (
			(HANDLE) 0xFFFFFFFF,
			NULL,
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
   
   printf("Mapping view of section\n");
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

