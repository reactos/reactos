/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>

int
fprintf(register FILE *iop, const char *fmt, ...)
{
  int len;
  char localbuf[BUFSIZ];
  va_list a=0;
  

  va_start( a, fmt ); 
  if (iop->_flag & _IONBF)
  {
    iop->_flag &= ~_IONBF;
    iop->_ptr = iop->_base = localbuf;
    iop->_bufsiz = BUFSIZ;
    len = _doprnt(fmt,a, iop);
    fflush(iop);
    iop->_flag |= _IONBF;
    iop->_base = NULL;
    iop->_bufsiz = 0;
    iop->_cnt = 0;
  }
  else
    len = _doprnt(fmt, a, iop);
  return ferror(iop) ? EOF : len;
}
