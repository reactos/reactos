/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/string.h>

wchar_t * wcscpy(wchar_t * str1,const wchar_t * str2)
{
  wchar_t *save = str1;

  for (; (*str1 = *str2); ++str2, ++str1);
  return save;
}
