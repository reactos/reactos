
#include <stddef.h>
#include <tchar.h>

size_t __cdecl _tcsnlen(const _TCHAR * str, size_t count)
{
 const _TCHAR * s = str;

 if(s == 0) return 0;

 while ( *s && count ) { ++ s, -- count; };

 return s - str;
}

/* EOF */
