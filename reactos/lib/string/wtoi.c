#include <string.h>
#include <stdlib.h>

/*
 * @implemented
 */
int
_wtoi(const wchar_t *str)
{
  return (int)wcstol(str, 0, 10);
}
