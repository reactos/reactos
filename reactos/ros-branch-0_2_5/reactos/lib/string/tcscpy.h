/* $Id: tcscpy.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <tchar.h>

_TCHAR * _tcscpy(_TCHAR * to, const _TCHAR * from)
{
 _TCHAR *save = to;

 for (; (*to = *from); ++from, ++to);

 return save;
}
