/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <conio.h>
#include <stdio.h>
#include <windows.h>

extern int char_avail;
extern int ungot_char;

int getch( void )
{
	return _getch();
}

int
_getch(void)
{
  
  DWORD  NumberOfCharsRead;
  char c;
  if (char_avail)
  {
    c = ungot_char;
    char_avail = 0;
  }
  else
  {
	
  	if( !ReadFile(filehnd(stdin->_file), &c,1,&NumberOfCharsRead ,NULL))
		return -1;

  }
  printk("%c",c);
  return c;
}