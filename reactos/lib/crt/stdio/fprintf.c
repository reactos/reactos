/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#include <wchar.h>
#include <tchar.h>

/*
 * @implemented
 */
int
_ftprintf(register FILE *iop, const _TCHAR *fmt, ...)
{
  int len;
  _TCHAR localbuf[BUFSIZ];
  va_list a=0;


  va_start( a, fmt );
  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = (char *)localbuf;
    iop->_bufsiz = BUFSIZ;
    len = _vftprintf(iop,fmt,a);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = _vftprintf(iop, fmt, a);
  return ferror(iop) ? -1 : len;
}
