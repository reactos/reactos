#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>

/* We support TLS cleanup code in any case. If shared version of libgcc is used _CRT_MT has value 1, otherwise
   we do tls cleanup in runtime and _CRT_MT has value 2.  */
int _CRT_MT = 2;

int __mingwthr_key_dtor (DWORD key, void (*dtor)(void *));
int __mingwthr_remove_key_dtor (DWORD key);

extern int ___w64_mingwthr_remove_key_dtor (DWORD key);
extern int ___w64_mingwthr_add_key_dtor (DWORD key, void (*dtor)(void *));

int
__mingwthr_remove_key_dtor (DWORD key)
{
   return ___w64_mingwthr_remove_key_dtor (key);
}

int
__mingwthr_key_dtor (DWORD key, void (*dtor)(void *))
{
  if (dtor)
    return ___w64_mingwthr_add_key_dtor (key, dtor);

  return 0;
}
