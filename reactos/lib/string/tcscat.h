/* $Id: tcscat.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
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
