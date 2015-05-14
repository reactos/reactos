
#include <tchar.h>

#ifdef _MSC_VER
#pragma function(_tcscat)
#endif /* _MSC_VER */

_TCHAR * _tcscat(_TCHAR * s, const _TCHAR * append)
{
 _TCHAR * save = s;

 for(; *s; ++s);

 while((*s++ = *append++));

 return save;
}

/* EOF */
