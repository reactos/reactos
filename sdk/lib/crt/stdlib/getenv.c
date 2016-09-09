/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/stdlib/getenv.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
//#undef environ

/*
 * @implemented
 */
char *getenv(const char *name)
{
   char **env;
   size_t length = strlen(name);

   for (env = *__p__environ(); *env; env++)
   {
      char *str = *env;
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
   wchar_t **env;
   size_t length = wcslen(name);

   for (env = *__p__wenviron(); *env; env++)
   {
      wchar_t *str = *env;
      wchar_t *pos = wcschr(str, L'=');
      if (pos && ((unsigned int)(pos - str) == length) && !_wcsnicmp(str, name, length))
         return pos + 1;
   }
   return NULL;
}
