/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <libc/file.h>
#include <internal/debug.h>
int
printf(const char *fmt, ...)
{
  int len;

  len = _doprnt(fmt, (&fmt)+1, stdout);

  /* People were confused when printf() didn't flush stdout,
     so we'll do it to reduce confusion */
  if (stdout->_flag & _IOLBF)
    fflush(stdout);

  return ferror(stdout) ? EOF : len;
}

int wprintf(const wchar_t *fmt, ...)
{
  int len;

  len = _dowprnt(fmt, (&fmt)+sizeof(wchar_t), stdout);

  /* People were confused when printf() didn't flush stdout,
     so we'll do it to reduce confusion */
  if (stdout->_flag & _IOLBF)
    fflush(stdout);

  return ferror(stdout) ? EOF : len;
}
