/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
//#include <crtdll/stubs.h>
#include <crtdll/stdio.h>
//#include <crtdll/unistd.h>
#include <crtdll/internal/file.h>
#include <crtdll/fcntl.h>
#include <crtdll/io.h>


long
ftell(FILE *f)
{
  long tres;
  int adjust=0;

  if (f->_cnt < 0)
    f->_cnt = 0;
  if (f->_flag&_IOREAD)
  {
    adjust = - f->_cnt;
  }
  else if (f->_flag&(_IOWRT|_IORW))
  {
    if (f->_flag&_IOWRT && f->_base && (f->_flag&_IONBF)==0)
      adjust = f->_ptr - f->_base;
  }
  else
    return -1;
  tres = lseek(fileno(f), 0L, 1);
  if (tres<0)
    return tres;
  tres += adjust;
  return tres;
}
