#include "precomp.h"
#include <stdlib.h>

#define NDEBUG
#include <internal/debug.h>

#undef environ

/*
 * @implemented
 */
char *getenv(const char *name)
{
   char **environ;
   unsigned int length = strlen(name);

   for (environ = *__p__environ(); *environ; environ++)
   {
      char *str = *environ;
      char *pos = strchr(str,'=');
      if (pos && ((unsigned int)(pos - str) == length) && !_strnicmp(str, name, length))
         return pos + 1;
   }
   return NULL;
}

/*
 * @implemented
 */
wchar_t *_wgetenv(const wchar_t *name)
{
   wchar_t **environ;
   unsigned int length = wcslen(name);

   for (environ = *__p__wenviron(); *environ; environ++)
   {
      wchar_t *str = *environ;
      wchar_t *pos = wcschr(str, L'=');
      if (pos && ((unsigned int)(pos - str) == length) && !_wcsnicmp(str, name, length))
         return pos + 1;
   }
   return NULL;
}
