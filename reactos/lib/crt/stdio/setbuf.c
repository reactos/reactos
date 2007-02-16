/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

/*
 * @implemented
 */
void setbuf(FILE *f, char *buf)
{
  if (buf)
    setvbuf(f, buf, _IOFBF, BUFSIZ);
  else
    setvbuf(f, 0, _IONBF, BUFSIZ);
}
