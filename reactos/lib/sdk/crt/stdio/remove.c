#include <precomp.h>
#include <tchar.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _tremove(const _TCHAR *fn)
{
  int result = 0;
  DPRINT(MK_STR(_tremove)"('%"sT"')\n", fn);
  if (!DeleteFile(fn))
    result = -1;
  DPRINT("%d\n", result);
  return result;
}

