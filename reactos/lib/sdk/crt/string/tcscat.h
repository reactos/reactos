/* $Id: tcscat.h 30283 2007-11-08 21:06:20Z fireball $
 */

#include <tchar.h>

_TCHAR * _tcscat(_TCHAR * s, const _TCHAR * append)
{
 _TCHAR * save = s;

 for(; *s; ++s);

 while((*s++ = *append++));

 return save;
}

/* EOF */
