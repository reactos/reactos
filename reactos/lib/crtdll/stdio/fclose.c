/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
//#include <crtdll/stubs.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h>
#include <crtdll/sys/types.h>
#include <crtdll/sys/stat.h>
#include <crtdll/stdlib.h>
//#include <crtdll/unistd.h>
#include <crtdll/internal/file.h>

#if 0
#ifndef __dj_include_stdio_h_
#define _name_to_remove _tmpfname
#endif
#endif

int
fclose(FILE *f)
{
  int r;

  r = EOF;
  if (!f)
    return r;
  if (f->_flag & (_IOREAD|_IOWRT|_IORW)
      && !(f->_flag&_IOSTRG))
  {
    r = fflush(f);
    if (_close(fileno(f)) < 0)
      r = EOF;
    if (f->_flag&_IOMYBUF)
      free(f->_base);
  }
  if (f->_flag & _IORMONCL && f->_name_to_remove)
  {
    remove(f->_name_to_remove);
    free(f->_name_to_remove);
    f->_name_to_remove = 0;
  }
  f->_cnt = 0;
  f->_base = 0;
  f->_ptr = 0;
  f->_bufsiz = 0;
  f->_flag = 0;
  f->_file = -1;
  return r;
}
