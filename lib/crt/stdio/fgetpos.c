/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
int fgetpos(FILE *stream, fpos_t *pos)
{
  if (stream && pos)
    {
      *pos = (fpos_t)ftell(stream);
      return 0;
    }
  //__set_errno ( EFAULT );
  return 1;
}
