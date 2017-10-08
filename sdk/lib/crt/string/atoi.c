/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
int
CDECL
_ttoi(const _TCHAR *str)
{
  return (int)_ttoi64(str);
}
