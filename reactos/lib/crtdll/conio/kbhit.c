/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/kbhit.c
 * PURPOSE:     Checks for keyboard hits
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <windows.h>
#include <conio.h>


// FIXME PeekCosoleInput returns more than keyboard hits
int
_kbhit(void)
{
  INPUT_RECORD InputRecord;
  DWORD NumberRead;
  if (char_avail)
    	return(1);
  else {
	PeekConsoleInput(stdin->file,&InputRecord,1,&NumberRead);
	return NumberRead;
  }
  return 0;
}
