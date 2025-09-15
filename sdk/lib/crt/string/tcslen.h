
#include <stddef.h>
#include <tchar.h>

#ifdef _MSC_VER
#pragma function(_tcslen)
#endif /* _MSC_VER */

size_t __cdecl _tcslen(const _TCHAR * str)
{
 const _TCHAR * s;

 /* Standard requires str to be non-NULL, no check needed */
 for(s = str; *s; ++ s);

 return s - str;
}

/* EOF */
