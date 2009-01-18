#define _DISABLE_TIDENTS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

int main( int argc, char **argv ) {
  ULONG ResultSize;
  PWCHAR WcharResult;
  WCHAR ValueNameWC[100];
  PCHAR CharResult;
  HKEY RegKey;
  int i;

  if( argc < 2 ) {
    printf( "Usage: regqueryvalue [key] [value]\n" );
    printf( "Returns an HKEY_LOCAL_MACHINE value from the given key.\n" );
    return 1;
  }

  if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, argv[1], 0, KEY_READ, &RegKey )
      != 0 ) {
    printf( "Could not open key %s\n", argv[1] );
    return 2;
  }

  for( i = 0; argv[2][i]; i++ ) ValueNameWC[i] = argv[2][i];
  ValueNameWC[i] = 0;

  if(RegQueryValueExW( RegKey, ValueNameWC, NULL, NULL, NULL, &ResultSize )
     != 0) {
    printf( "The value %S does not exist.\n", ValueNameWC );
    return 5;
  }

  WcharResult = malloc( (ResultSize + 1) * sizeof(WCHAR) );

  if( !WcharResult ) {
    printf( "Could not alloc %d wchars\n", (int)(ResultSize + 1) );
    return 6;
  }

  RegQueryValueExW( RegKey, ValueNameWC, NULL, NULL, (LPBYTE)WcharResult,
		    &ResultSize );

  printf( "wchar Value: %S\n", WcharResult );
  fflush( stdout );

  RegQueryValueExA( RegKey, argv[2], NULL, NULL, NULL, &ResultSize );

  CharResult = malloc( ResultSize + 1 );

  if( !CharResult ) {
    printf( "Could not alloc %d chars\n", (int)(ResultSize + 1) );
    return 7;
  }

  RegQueryValueExA( RegKey, argv[2], NULL, NULL, (PBYTE)CharResult, &ResultSize );

  printf( " char Value: %s\n", CharResult );
  fflush( stdout );

  free( WcharResult );
  free( CharResult );
  free( ValueNameWC );

  return 0;
}
