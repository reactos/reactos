#include "precomp.h"
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

/* misc/environ.c */
int SetEnv(const wchar_t *option);

/*
 * @implemented
 */
int _wputenv(const wchar_t* val)
{
   return SetEnv(val);
}
