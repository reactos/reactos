#include <stdio.h>
typedef int (__cdecl *PFN_sntprintf)(char *_Dest, size_t _Count, const char *_Format, ...);
#define str_sntprintf "_snprintf"
#include "_sntprintf.h"

START_TEST(_snprintf)
{
    Test__sntprintf();
}
