
#include <msvcrt/string.h>

/*
 * @unimplemented
 */
int wcscoll(const wchar_t *a1,const wchar_t *a2)
{
  /* FIXME: handle collates */
  return wcscmp(a1,a2);
}

/*
 * @unimplemented
 */
int _wcsicoll(const wchar_t *a1,const wchar_t *a2)
{
  /* FIXME: handle collates */
  return _wcsicmp(a1,a2);
}

/*
 * @unimplemented
 */
int _wcsncoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return wcsncmp(s1,s2,c);
}

/*
 * @unimplemented
 */
int _wcsnicoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return _wcsnicmp(s1,s2,c);
}
