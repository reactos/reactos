/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

#ifdef ferror
#undef ferror
int ferror(FILE *stream);
#endif

int ferror(FILE *stream)
{
  return stream->_flag & _IOERR;
}
