/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <tchar.h>

/*
 * @implemented
 */
long
_ttol(const _TCHAR *str)
{
  return _tcstol(str, 0, 10);
}
