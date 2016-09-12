
#include <tchar.h>

#ifdef _MSC_VER
#pragma function(_tcscpy)
#endif /* _MSC_VER */

_TCHAR * _tcscpy(_TCHAR * to, const _TCHAR * from)
{
 _TCHAR *save = to;

 for (; (*to = *from); ++from, ++to);

 return save;
}
