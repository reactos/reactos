#include <windows.h>
#include <msvcrt/stddef.h>
#include <msvcrt/stdio.h>

int remove(const char *fn)
{
  if (!DeleteFileA(fn))
    return -1;
  return 0;
}

int _wremove(const wchar_t *fn)
{
  if (!DeleteFileW(fn))
    return -1;
  return 0;
}
