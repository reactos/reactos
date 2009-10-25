/* $Id$
 */

#include <tchar.h>

_TCHAR * _tcscpy(_TCHAR * to, const _TCHAR * from)
{
 _TCHAR *save = to;

 for (; (*to = *from); ++from, ++to);

 return save;
}
