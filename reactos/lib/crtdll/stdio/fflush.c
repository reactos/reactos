/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/stdio.h>
#include <crtdll/errno.h>
#include <crtdll/sys/types.h>
#include <crtdll/sys/stat.h>
#include <crtdll/stdlib.h>
#include <crtdll/internal/file.h>
#include <crtdll/io.h>


int
fflush(FILE *f)
{
  char *base;
  int n, rn;

  if (f == NULL)
  {
     int e = errno;

     __set_errno(0);
    _fwalk((void (*)(FILE *))fflush);
    if (_errno)
      return EOF;
    __set_errno(e);
    return 0;
  }

  f->_flag &= ~_IOUNGETC;
  if ((f->_flag&(_IONBF|_IOWRT))==_IOWRT
      && (base = f->_base) != NULL
      && (rn = n = f->_ptr - base) > 0)
  {
    f->_ptr = base;
    f->_cnt = (f->_flag&(_IOLBF|_IONBF)) ? 0 : f->_bufsiz;
    do {
      n = _write(fileno(f), base, rn);
      if (n <= 0) {
	f->_flag |= _IOERR;
	return EOF;
      }
      rn -= n;
      base += n;
    } while (rn > 0);
  }
  if (f->_flag & _IORW)
  {
    f->_cnt = 0;
    f->_flag &= ~(_IOWRT|_IOREAD);
    f->_ptr = f->_base;
  }
  return 0;
}
