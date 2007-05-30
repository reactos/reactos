/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#ifdef feof
#undef feof
int feof(FILE *stream);
#endif

/*
 * @implemented
 */
int feof(FILE *stream)
{
  if (stream == NULL) {
    __set_errno (EINVAL);
    return EOF;
  }

  return stream->_flag & _IOEOF;
}
