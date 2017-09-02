#include <precomp.h>

/*
 * @implemented
 */
double
_wtof(const wchar_t *str)
{
  return wcstod(str, 0);
}
