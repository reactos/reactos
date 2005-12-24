/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#include <tchar.h>

/*
 * @implemented
 */
int
_vstprintf(_TCHAR *str, const _TCHAR *fmt, va_list ap)
{
  FILE f = {0};
  int len;

  f._flag = _IOWRT|_IOSTRG|_IOBINARY;
  f._ptr = (char*)str;
  f._cnt = INT_MAX;
  f._file = -1;
  len = _vftprintf(&f,fmt, ap);
  *(_TCHAR*)f._ptr = 0;
  return len;
}


/*
 * @implemented
 */
int
_vsntprintf(_TCHAR *str, size_t maxlen, const _TCHAR *fmt, va_list ap)
{
  FILE f = {0};
  int len;

  f._flag = _IOWRT|_IOSTRG|_IOBINARY;
  f._ptr = (char*)str;
  f._cnt = maxlen * sizeof(_TCHAR);
  f._file = -1;
  len = _vftprintf(&f,fmt, ap);
  // what if the buffer is full ??
  *(_TCHAR *)f._ptr = 0;
  return len;
}
