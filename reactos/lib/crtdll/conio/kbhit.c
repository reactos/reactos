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
#include <crtdll/conio.h>
#include <crtdll/stdio.h>


// FIXME PeekCosoleInput returns more than keyboard hits
extern int char_avail;

int
_kbhit(void)
{
  INPUT_RECORD InputRecord;
  DWORD NumberRead=0;
  if (char_avail)
    	return(1);
  else {
	printf("fixeme PeekConsoleInput might do DeviceIo \n");
	//PeekConsoleInput((HANDLE)stdin->_file,&InputRecord,1,&NumberRead);
	return NumberRead;
  }
  return 0;
}
