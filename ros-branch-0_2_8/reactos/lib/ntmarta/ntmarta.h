#include <windows.h>
#include <accctrl.h>

#ifndef HAS_FN_PROGRESSW
#define FN_PROGRESSW FN_PROGRESS
#endif
#ifndef HAS_FN_PROGRESSA
#define FN_PROGRESSA FN_PROGRESS
#endif

ULONG DbgPrint(PCH Format,...);

extern HINSTANCE hDllInstance;

/* EOF */
