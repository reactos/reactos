/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/toupper.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */

#include <ctype.h>

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
wchar_t towupper(wchar_t c)
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

