/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef toupper
int toupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
#undef towupper
int towupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}

int _toupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}

int _towupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
