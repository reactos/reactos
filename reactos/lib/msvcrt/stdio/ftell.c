/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


long ftell(FILE *f)
{
  long tres;
  int adjust=0;

  if (!f)
    {
      __set_errno(EBADF);
      return -1;
    }

  if (f->_cnt < 0)
    f->_cnt = 0;
  else if (f->_flag&(_IOWRT))
    {
      if (f->_base && (f->_flag&_IONBF)==0)
        adjust = f->_ptr - f->_base;
    }
  else if (f->_flag&_IOREAD)
    {
      adjust = - f->_cnt;
    }
  else
    return -1;

  tres = _lseek(_fileno(f), 0L, SEEK_CUR);
  if (tres<0)
    return tres;
  tres += adjust;

  //f->_cnt = f->_bufsiz - tres;
  //f->_ptr = f->_base + tres;

  return tres;
}
