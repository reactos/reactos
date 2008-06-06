#include <precomp.h>
#include <tchar.h>

#ifdef _UNICODE
   #define sT "S"
#else
   #define sT "s"
#endif

#define MK_STR(s) #s

/*
 * @implemented
 */
int _tremove(const _TCHAR *fn)
{
  int result = 0;
  TRACE(MK_STR(_tremove)"('%"sT"')\n", fn);
  if (!DeleteFile(fn))
    result = -1;
  TRACE("%d\n", result);
  return result;
}


