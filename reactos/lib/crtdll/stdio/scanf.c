/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <stdarg.h>
#include <crtdll/internal/file.h>

int
scanf(const char *fmt, ...)
{
  int r;
  va_list a=0;
  va_start(a, fmt);
  r = _doscan(stdin, fmt, a);
  va_end(a);
  return r;
}

int
wscanf(const wchar_t *fmt, ...)
{
  int r;
  va_list a=0;
  va_start(a, fmt);
  r = _dowscan(stdin, fmt, a);
  va_end(a);
  return r;
}

