/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <windows.h>
#include <crtdll/internal/file.h>

#include <crtdll/crtdll.h>

int putc(int c, FILE *fp)
{ 
   if (fp->_cnt > 0 ) 
     {
	fp->_cnt--;
	*(fp)->_ptr++ = (char)c;
	return (int)c; 
     }
   else
     return  _flsbuf(c,fp);
   
   return -1;
}
