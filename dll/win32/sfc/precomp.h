#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <windows.h>
#include <sfc.h>
#include <srrestoreptapi.h>

DWORD WINAPI sfc_8(VOID);
DWORD WINAPI sfc_9(VOID);

typedef BOOL (WINAPI *PSRSRPA)(PRESTOREPOINTINFOA, PSTATEMGRSTATUS);
typedef BOOL (WINAPI *PSRSRPW)(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);

#endif
