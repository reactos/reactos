#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#include <windows.h>
#include <ndk/ntndk.h>
#include <accctrl.h>
#include <aclapi.h>
#include <winsvc.h>
#include <ntsecapi.h>

#ifndef HAS_FN_PROGRESSW
#define FN_PROGRESSW FN_PROGRESS
#endif
#ifndef HAS_FN_PROGRESSA
#define FN_PROGRESSA FN_PROGRESS
#endif

ULONG DbgPrint(PCCH Format,...);

extern HINSTANCE hDllInstance;

/* EOF */
