
#include <msvcrt/string.h>

int wcscoll(const wchar_t *a1,const wchar_t *a2)
{
  /* FIXME: handle collates */
  return wcscmp(a1,a2);
}

int _wcsicoll(const wchar_t *a1,const wchar_t *a2)
{
  /* FIXME: handle collates */
  return _wcsicmp(a1,a2);
}

int _wcsncoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return wcsncmp(s1,s2,c);
}

int _wcsnicoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return _wcsnicmp(s1,s2,c);
}
