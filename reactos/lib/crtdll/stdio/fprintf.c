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
