/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
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
   char **environ;
   size_t length = strlen(name);

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
   size_t length = wcslen(name);

   for (environ = *__p__wenviron(); *environ; environ++)
   {
      wchar_t *str = *environ;
      wchar_t *pos = wcschr(str, L'=');
      if (pos && ((unsigned int)(pos - str) == length) && !_wcsnicmp(str, name, length))
         return pos + 1;
   }
   return NULL;
}
