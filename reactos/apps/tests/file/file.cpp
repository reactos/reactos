/***********************************************************
 * File read/write test utility                            *
 **********************************************************/

#include <windows.h>
#include <iostream>
#include <stdlib.h>

int main( void )
{
  HANDLE file = CreateFile( "test.dat", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
  char buffer[4096];

  if( file == INVALID_HANDLE_VALUE )
  {
    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, buffer, 4096, NULL );
    cout << "Error opening file: " << buffer << endl;
    return 1;
  }
  DWORD wrote;
  for( int c = 0; c < 1024; c++ )
    if( WriteFile( file, buffer, 4096, &wrote, NULL ) == FALSE )
    {
      FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, buffer, 4096, NULL );
      cout << "Error writing file: " << buffer << endl;
      exit(2);
    }
  cout << "File written, trying to read now" << endl;
  for( int c = 0; c < 1024; c++ )
    if( ReadFile( file, buffer, 4096, &wrote, NULL ) == FALSE )
    {
      FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, buffer, 4096, NULL );
      cout << "Error reading file: " << buffer << endl;
      exit(3);
    }
  cout << "Test passed" << endl;

  return 0;
}
