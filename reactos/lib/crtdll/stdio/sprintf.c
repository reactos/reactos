/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <limits.h>
#include <libc/file.h>


int
sprintf(char *str, const char *fmt, ...)
{
  FILE _strbuf;
  int len;

  _strbuf._flag = _IOWRT|_IOSTRG;
  _strbuf._ptr = str;
  _strbuf._cnt = INT_MAX;
  len = _doprnt(fmt, &(fmt)+1, &_strbuf);
  *_strbuf._ptr = 0;
  return len;
}

swprintf(wchar_t *str, const wchar_t *fmt, ...)
{
  FILE _strbuf;
  int len;

  _strbuf._flag = _IOWRT|_IOSTRG;
  _strbuf._ptr = str;
  _strbuf._cnt = INT_MAX;
  len = _dowprnt(fmt,(va_list) &(fmt)+sizeof(wchar_t), &_strbuf);
  *_strbuf._ptr = 0;
  return len;
}
