#include "precomp.h"
#include <stdlib.h>
#include <string.h>

#define NDEBUG
#include <internal/msvcrtdbg.h>

/* misc/environ.c */
int SetEnv(const wchar_t *option);

/*
 * @implemented
 */
int _wputenv(const wchar_t* val)
{
   return SetEnv(val);
}
