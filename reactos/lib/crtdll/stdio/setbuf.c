/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/internal/file.h>

void
setbuf(FILE *f, char *buf)
{
  if (buf)
    setvbuf(f, buf, _IOFBF, BUFSIZ);
  else
    setvbuf(f, 0, _IONBF, BUFSIZ);
}
