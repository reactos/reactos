/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


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
   return (c + ('A' - 'a'));
}

/*
wchar_t _towupper(wchar_t c)
{
   return (c + (L'A' - L'a'));
}
*/

