#include <string.h>
#include <stdlib.h>

/*
 * @implemented
 */
long
atol(const char *str)
{
  return strtol(str, 0, 10);
}
