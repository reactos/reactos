/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

#undef ferror
int
ferror(FILE *stream)
{
  return stream->_flag & _IOERR;
}
