/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

/*
 * @implemented
 */
long
_wtol(const wchar_t *str)
{
  return wcstol(str, 0, 10);
}
