/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

int
fsetpos(FILE *stream,const fpos_t *pos)
{
  if (stream && pos)
  {
    fseek(stream, (long)(*pos), SEEK_SET);
    return 0;
  }
  __set_errno(EFAULT);
  return -1;
}
