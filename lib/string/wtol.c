#include <string.h>
#include <stdlib.h>

/*
 * @implemented
 */
long
_wtol(const wchar_t *str)
{
  return wcstol(str, 0, 10);
}
