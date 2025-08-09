
#include <stddef.h>
#include <tchar.h>

#ifdef _MSC_VER
#pragma function(_tcslen)
#endif /* _MSC_VER */

size_t __cdecl _tcslen(const _TCHAR * str)
{
 const _TCHAR * s;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
 if(str == 0) return 0;
#pragma GCC diagnostic pop

 for(s = str; *s; ++ s);

 return s - str;
}

/* EOF */
