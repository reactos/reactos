/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int _unMode_dll = 022;

int _umask (int unMode)
{
  unsigned old_mask = _unMode_dll;
  _unMode_dll = unMode;
  return old_mask;
}
