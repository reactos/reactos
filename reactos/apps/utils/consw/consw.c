/* $Id$
 *
 * DESCRIPTION: Console mode switcher
 * PROGRAMMER:  Art Yerkes
 * REVISIONS
 * 	2003-07-26 (arty)
 */

#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void STDCALL SetConsoleHardwareState( HANDLE conhandle,
				      DWORD flags,
				      DWORD state );

int main(int argc, char* argv[])
{
  if( argc > 1 ) {
    SetConsoleHardwareState( GetStdHandle( STD_INPUT_HANDLE ), 
			     0,
			     !strcmp( argv[1], "hw" ) );
  }
  return 0;
}


/* EOF */
