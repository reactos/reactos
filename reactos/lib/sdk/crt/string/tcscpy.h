/* $Id: tcscpy.h 30283 2007-11-08 21:06:20Z fireball $
 */

#include <tchar.h>

_TCHAR * _tcscpy(_TCHAR * to, const _TCHAR * from)
{
 _TCHAR *save = to;

 for (; (*to = *from); ++from, ++to);

 return save;
}
