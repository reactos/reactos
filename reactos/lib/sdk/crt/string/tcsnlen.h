/* $Id: tcsnlen.h 38052 2008-12-13 21:28:05Z tkreuzer $
 */

#include <stddef.h>
#include <tchar.h>

size_t _tcsnlen(const _TCHAR * str, size_t count)
{
 const _TCHAR * s;

 if(str == 0) return 0;

 for(s = str; *s && count; ++ s, -- count);

 return s - str;
}

/* EOF */
