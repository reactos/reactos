/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

#undef	clearerr
void
clearerr(FILE *f)
{
  f->_flag &= ~(_IOERR|_IOEOF);
}
