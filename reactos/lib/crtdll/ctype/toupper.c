/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/ctype.h>
#include <crtdll/wchar.h>

#undef toupper
int toupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
#undef towupper
wchar_t towupper(wchar_t c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}

int _toupper(int c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}

wchar_t _towupper(wchar_t c)
{
  return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
