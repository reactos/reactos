/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


#undef toupper
/*
 * @implemented
 */
int toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + ('A' - 'a'));
   return(c);
}

#undef towupper
/*
 * @implemented
 */
int towupper(wint_t c)
{
   if (iswctype (c, _LOWER))
      return (c + (L'A' - L'a'));
   return(c);
}

/*
 * @implemented
 */
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

