/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <io.h>
#include <windows.h>
#include <libc/file.h>


int getc(FILE *f)
{
  int c;
  DWORD NumberOfBytesRead;
  if ( !ReadFile(_get_osfhandle(f->_file),&c, 1, &NumberOfBytesRead,	NULL) )
	return -1;
  if ( NumberOfBytesRead == 0 )
	  return -1;
  return c;
}
