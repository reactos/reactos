/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
//#include <libc/stubs.h>
#include <stdio.h>
#include <stdlib.h>

void setbuffer(FILE *f, void *buf, int size)
{
  if (buf)
    setvbuf(f, buf, _IOFBF, size);
  else
    setvbuf(f, 0, _IONBF, 0);
}
