/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <libc/file.h>

#define _fillsize _bufsiz

int setvbuf(FILE *f, char *buf, int type, size_t len)
{
  int mine=0;
  if (!f)
    return -1;
  fflush(f);
  switch (type)
  {
  case _IOFBF:
  case _IOLBF:
    if (len <= 0)
      return -1;
    if (buf == 0)
    {
      buf = (char *)malloc(len);
      if (buf == 0)
	return -1;
      mine = 1;
    }
    f->_fillsize = len;	/* make it read in `len'-byte chunks */
    /* FALLTHROUGH */
  case _IONBF:
    if (f->_base != NULL && f->_flag & _IOMYBUF)
      free(f->_base);
    f->_cnt = 0;

    f->_flag &= ~(_IONBF|_IOFBF|_IOLBF|_IOUNGETC);
    f->_flag |= type;
    if (type != _IONBF)
    {
      if (mine)
	f->_flag |= _IOMYBUF;
      f->_ptr = f->_base = buf;
      f->_bufsiz = len;
    }
    else
    {
      f->_base = 0;
      f->_bufsiz = 0;
    }
    return 0;
  default:
    return -1;
  }
}
