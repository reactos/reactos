/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

#ifdef feof
#undef feof
feof(FILE *stream);
#endif

feof(FILE *stream)
{
  return stream->_flag & _IOEOF;
}
