/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <msvcrt/internal/file.h>

int
vsprintf(char *str, const char *fmt, va_list ap)
{
  FILE f;
  int len;

  f._flag = _IOWRT|_IOSTRG;
  f._ptr = str;
  f._cnt = INT_MAX;
  f._file = -1;
  len = vfprintf(&f,fmt, ap);
  *f._ptr = 0;
  return len;
}

int
vswprintf(wchar_t *str, const wchar_t *fmt, va_list ap)
{
  FILE f;
  int len;

  f._flag = _IOWRT|_IOSTRG;
  f._ptr = (char*)str;
  f._cnt = INT_MAX;
  f._file = -1;
  len = vfwprintf(&f,fmt, ap);
  *f._ptr = 0;
  return len;
}


int
_vsnprintf(char *str, size_t maxlen, const char *fmt, va_list ap)
{
  FILE f;
  int len;
  f._flag = _IOWRT|_IOSTRG;
  f._ptr = str;
  f._cnt = maxlen;
  f._file = -1;
  len = vfprintf(&f,fmt, ap);
  // what if the buffer is full ??
  *f._ptr = 0;
  return len;
}

int
_vsnwprintf(wchar_t *str, size_t maxlen, const wchar_t *fmt, va_list ap)
{
  FILE f;
  int len;
  f._flag = _IOWRT|_IOSTRG;
  f._ptr = (char*)str;
  f._cnt = maxlen;
  f._file = -1;
  len = vfwprintf(&f,fmt, ap);
  // what if the buffer is full ??
  *f._ptr = 0;
  return len;
}


