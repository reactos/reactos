/* $Id$
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
