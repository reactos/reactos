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

/* misc/environ.c */
int SetEnv(const wchar_t *option);

/*
 * @implemented
 */
int _putenv(const char* val)
{
   int size, result;
   wchar_t *woption;

   size = MultiByteToWideChar(CP_ACP, 0, val, -1, NULL, 0);
   woption = malloc(size* sizeof(wchar_t));
   if (woption == NULL)
      return -1;
   MultiByteToWideChar(CP_ACP, 0, val, -1, woption, size);
   result = SetEnv(woption);
   free(woption);
   return result;
}
