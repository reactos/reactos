/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
//#include <unconst.h>

/*
 * @implemented
 */
char *
strstr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0)
  {
    len = strlen(find);
    do {
      do {
	if ((sc = *s++) == 0)
	  return 0;
      } while (sc != c);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return (char *)s;
}
