/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <stdarg.h>
#include <crtdll/internal/file.h>
//#include <crtdll/unconst.h>

int
sscanf(const char *str, const char *fmt, ...)
{
  int r;
  va_list a=0;
  FILE _strbuf;

  va_start(a, fmt);

  _strbuf._flag = _IOREAD|_IOSTRG;
  _strbuf._ptr = (char *)str;
  _strbuf._base = (char *)str;
  _strbuf._cnt = 0;
  while (*str++)
    _strbuf._cnt++;
  _strbuf._bufsiz = _strbuf._cnt;
  r = _doscan(&_strbuf, fmt, a);
  va_end(a);
  return r;
}

int
swscanf(const wchar_t *str, const wchar_t *fmt, ...)
{
  int r;
  va_list a=0;
  FILE _strbuf;

  va_start(a, fmt);

  _strbuf._flag = _IOREAD|_IOSTRG;
  _strbuf._ptr = (char *)str;
  _strbuf._base = (char *)str;
  _strbuf._cnt = 0;
  while (*str++)
    _strbuf._cnt++;
  _strbuf._bufsiz = _strbuf._cnt;
  r = _dowscan(&_strbuf, fmt, a);
  va_end(a);
  return r;
}
