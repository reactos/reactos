/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


void rewind(FILE *f)
{
  fflush(f);
  _lseek(_fileno(f), 0L, SEEK_SET);
  f->_cnt = 0;
  f->_ptr = f->_base;
  f->_flag &= ~(_IOERR|_IOEOF|_IOAHEAD);
}
