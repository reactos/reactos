/***********************************************************
 * File read/write test utility                            *
 **********************************************************/

#include <windows.h>
#include <stdlib.h>

int main()
{
   HANDLE file;
   char buffer[4096];
   DWORD wrote;
   int c;
   
   file = CreateFile("test.dat", 
		     GENERIC_READ | GENERIC_WRITE, 
		     0, 
		     NULL, 
		     CREATE_ALWAYS, 
		     0, 
		     0);
   
  if (file == INVALID_HANDLE_VALUE)
     {
	printf("Error opening file (Status %x)\n", GetLastError());
	return 1;
     }
      
   printf("Writing file\n");
   for (c = 0; c < 1024; c++)
     {
	if (WriteFile( file, buffer, 4096, &wrote, NULL) == FALSE)
	  {
	     printf("Error writing file (Status %x)\n", GetLastError());
	     exit(2);
	  }
    }
   
   printf("Reading file\n");
   for (c = 0; c < 1024; c++)
     {
	if (ReadFile( file, buffer, 4096, &wrote, NULL) == FALSE)
	  {
	     printf("Error reading file (Status %x)\n", GetLastError());
	     exit(3);
	  }
     }
   
   printf("Finished\n");
}
