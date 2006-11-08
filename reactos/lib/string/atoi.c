#include <string.h>
#include <stdlib.h>

/*
 * @implemented
 */
int
atoi(const char *str)
{
  return (int)strtol(str, 0, 10);
}
