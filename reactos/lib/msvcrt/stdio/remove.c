#include <windows.h>
#include <msvcrt/stddef.h>
#include <msvcrt/stdio.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

int remove(const char *fn)
{
  int result = 0;
  DPRINT("remove('%s')\n", fn);
  if (!DeleteFileA(fn))
    result = -1;
  DPRINT("%d\n", result);
  return result;
}

int _wremove(const wchar_t *fn)
{
  DPRINT("_wremove('%S')\n", fn);
  if (!DeleteFileW(fn))
    return -1;
  return 0;
}
