/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

int
fprintf(register FILE *iop, const char *fmt, ...)
{
  int len;
  char localbuf[BUFSIZ];

  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = localbuf;
    iop->_bufsiz = BUFSIZ;
    len = _doprnt(fmt, (&fmt)+1, iop);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = _doprnt(fmt, (&fmt)+1, iop);
  return ferror(iop) ? EOF : len;
}

int
fwprintf(register FILE *iop, const wchar_t *fmt, ...)
{
  int len;
  wchar_t localbuf[BUFSIZ];

  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = localbuf;
    iop->_bufsiz = BUFSIZ;
    len = _dowprnt(fmt, (&fmt)+sizeof(wchar_t), iop);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = _dowprnt(fmt, (va_list)(&fmt)+sizeof(wchar_t), iop);
  return ferror(iop) ? EOF : len;
}
