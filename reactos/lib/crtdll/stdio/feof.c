/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>

#ifdef feof
#undef feof
int feof(FILE *stream);
#endif

int feof(FILE *stream)
{
  return stream->_flag & _IOEOF;
}
