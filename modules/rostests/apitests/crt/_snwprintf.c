#define _UNICODE
#include <wchar.h>
typedef int (__cdecl *PFN_sntprintf)(wchar_t *_Dest, size_t _Count, const wchar_t *_Format, ...);
#define str_sntprintf "_snwprintf"
#include "_sntprintf.h"

START_TEST(_snwprintf)
{
    Test__sntprintf();
}
