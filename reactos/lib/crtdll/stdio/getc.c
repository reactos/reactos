/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <windows.h>
#include <crtdll/internal/file.h>

int getc(FILE *f)
{
  int c;
  DWORD NumberOfBytesRead =0;
  if (f->_cnt > 0) {
	f->_cnt--;
	f->_flag &= ~_IOUNGETC;
	return  *f->_ptr;
  }
	
 
  if ( !ReadFile(_get_osfhandle(f->_file),&c, 1, &NumberOfBytesRead,	NULL) )
	return -1;
  if ( NumberOfBytesRead == 0 )
	  return -1;
  putchar(c&0xFF);
  return c&0xFF;

 
}


