/* $Id: tcsnlen.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <stddef.h>
#include <tchar.h>

int _tcsnlen(const _TCHAR * str, size_t count)
{
 const _TCHAR * s;

 if(str == 0) return 0;

 for(s = str; *s && count; ++ s, -- count);

 return s - str;
}

/* EOF */
