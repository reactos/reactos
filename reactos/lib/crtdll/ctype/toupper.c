/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/ctype.h>
#include <crtdll/wchar.h>

#undef toupper
int toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + ('A' - 'a'));
   return(c);
}

#undef towupper
wchar_t towupper(wchar_t c)
{
   if (iswctype (c, _LOWER))
      return (c + (L'A' - L'a'));
   return(c);
}

int _toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + ('A' - 'a'));
   return(c);
}

wchar_t _towupper(wchar_t c)
{
   if (iswctype (c, _LOWER))
      return (c + (L'A' - L'a'));
   return(c);
}
