/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <errno.h>

int
fsetpos(FILE *stream, fpos_t *pos)
{
  if (stream && pos)
  {
    fseek(stream, (long)(*pos), SEEK_SET);
    return 0;
  }
  //errno = EFAULT;
  return 1;
}
