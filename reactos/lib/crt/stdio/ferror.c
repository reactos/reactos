/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <internal/file.h>

#ifdef ferror
#undef ferror
int ferror(FILE *stream);
#endif

int *_errno(void);

/*
 * @implemented
 */
int ferror(FILE *stream)
{
  return stream->_flag & _IOERR;
}
