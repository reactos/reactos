/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/sys/stat.h>

unsigned _unMode_dll = 022;

/*
 * @implemented
 */
unsigned _umask (unsigned unMode)
{
  unsigned old_mask = _unMode_dll;
  _unMode_dll = unMode;
  return old_mask;
}
