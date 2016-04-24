#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#define NTOS_MODE_USER
#include <windows.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <aclapi.h>
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
