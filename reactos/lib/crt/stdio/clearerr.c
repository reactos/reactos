/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

#ifdef clearerr
#undef clearerr
void clearerr(FILE *stream);
#endif

/*
 * @implemented
 */
void
clearerr(FILE *f)
{
  if (!__validfp (f)) {
    __set_errno (EINVAL);
    return;
  }
  f->_flag &= ~(_IOERR|_IOEOF);
}
