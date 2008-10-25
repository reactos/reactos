#include <precomp.h>


/* misc/environ.c */
int SetEnv(const wchar_t *option);

/*
 * @implemented
 */
int _wputenv(const wchar_t* val)
{
   return SetEnv(val);
}
