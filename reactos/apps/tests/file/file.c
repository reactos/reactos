/***********************************************************
 * File read/write test utility                            *
 **********************************************************/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

int main( void )
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
   for( c = 0; c < sizeof( buffer ); c++ )
      buffer[c] = (char)c;
   printf("Writing file\n");
   if (WriteFile( file, buffer, 4096, &wrote, NULL) == FALSE)
     {
       printf("Error writing file (Status %x)\n", GetLastError());
       exit(2);
       }
   printf("Reading file\n");
   SetFilePointer( file, 0, 0, FILE_BEGIN );
   if (ReadFile( file, buffer, 4096, &wrote, NULL) == FALSE)
     {
       printf("Error reading file (Status %x)\n", GetLastError());
       exit(3);
     }
   for( c = 0; c < sizeof( buffer ); c++ )
      if( buffer[c] != (char)c )
	 {
	    printf( "Error: data read back is not what was written\n" );
	    CloseHandle( file );
	    return 0;
	 }
   printf("Finished, works fine\n");
   CloseHandle( file );
   return 0;
}
