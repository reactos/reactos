/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>

#ifdef clearerr
#undef clearerr
void clearerr(FILE *stream);
#endif

void
clearerr(FILE *f)
{
  f->_flag &= ~(_IOERR|_IOEOF);
}
