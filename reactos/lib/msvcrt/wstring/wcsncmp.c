#include <msvcrt/wchar.h>

#if 0

int wcsncmp(const wchar_t* cs, const wchar_t* ct, size_t count)
{
  while ((*cs) == (*ct) && count > 0) {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
    count--;
  }
  return (*cs) - (*ct);
}

#else

int wcsncmp(const wchar_t* cs, const wchar_t* ct, size_t count)
{
  if (count == 0)
    return 0;
  do {
    if (*cs != *ct++)
      //return *(unsigned const char *)cs - *(unsigned const char *)--ct;
      return (*cs) - (*(--ct));
    if (*cs++ == 0)
      break;
  } while (--count != 0);
  return 0;
}

#endif
