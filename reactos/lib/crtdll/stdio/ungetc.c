/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>

int
ungetc(int c, FILE *f)
{
  if (c == EOF
      || (f->_flag & (_IOREAD|_IORW)) == 0
      || f->_ptr == NULL
      || f->_base == NULL)
    return EOF;

  if (f->_ptr == f->_base)
  {
    if (f->_cnt == 0)
      f->_ptr++;
    else
      return EOF;
  }

  f->_cnt++;
  f->_ptr--;
  if(*f->_ptr != c)
  {
    f->_flag |= _IOUNGETC;
    *f->_ptr = c;
  }

  return c;
}
