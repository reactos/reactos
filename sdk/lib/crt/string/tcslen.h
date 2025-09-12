
#include <stddef.h>
#include <tchar.h>

#ifdef _MSC_VER
#pragma function(_tcslen)
#endif /* _MSC_VER */

size_t __cdecl _tcslen(const _TCHAR * str)
{
 const _TCHAR * s;

 if(str == 0) return 0;

 for(s = str; *s; ++ s);

 return s - str;
}

/* EOF */
