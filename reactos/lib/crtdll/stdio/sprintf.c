/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <limits.h>
#include <crtdll/internal/file.h>


int
sprintf(char *str, const char *fmt, ...)
{
  FILE _strbuf;
  int len;
  va_list a = 0;
  va_start( a, fmt ); 

  _strbuf._flag = _IOWRT|_IOSTRG;
  _strbuf._ptr = str;
  _strbuf._cnt = INT_MAX;
  len = vfprintf(&_strbuf, fmt, a);
  *_strbuf._ptr = 0;
  return len;
}
