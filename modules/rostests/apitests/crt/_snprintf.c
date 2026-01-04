#include <stdio.h>
#define func__sntprintf func__snprintf
typedef int (__cdecl *PFN_sntprintf)(char *_Dest, size_t _Count, const char *_Format, ...);
#define str_sntprintf "_snprintf"
#include "_sntprintf.h"
