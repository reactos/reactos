/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>


int printf(const char *fmt, ...)
{
  int len;
  va_list a;
   
  va_start( a, fmt ); 
  len = _doprnt(fmt, a, stdout);

  /* People were confused when printf() didn't flush stdout,
     so we'll do it to reduce confusion */
  if (stdout->_flag & _IOLBF)
    fflush(stdout);

  return ferror(stdout) ? EOF : len;
}
