/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int
fputs(const char *s, FILE *f)
{
  int r = 0;
  int c;
  int unbuffered;
  char localbuf[BUFSIZ];

  unbuffered = f->_flag & _IONBF;
  if (unbuffered)
  {
    f->_flag &= ~_IONBF;
    f->_ptr = f->_base = localbuf;
    f->_bufsiz = BUFSIZ;
  }

  while ((c = *s++))
    r = putc(c, f);

  if (unbuffered)
  {
    fflush(f);
    f->_flag |= _IONBF;
    f->_base = NULL;
    f->_bufsiz = 0;
    f->_cnt = 0;
  }

  return(r);
}
