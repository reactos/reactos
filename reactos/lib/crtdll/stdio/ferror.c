/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

#ifdef ferror
#undef ferror
ferror(FILE *stream);
#endif

int
ferror(FILE *stream)
{
  return stream->_flag & _IOERR;
}
