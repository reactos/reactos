#ifndef IDNDL_H_
#define IDNDL_H_

#include <windows.h>

#ifndef GSS_ALLOW_INHERITED_COMMON
#define GSS_ALLOW_INHERITED_COMMON 1
#endif

#ifndef VS_ALLOW_LATIN
#define VS_ALLOW_LATIN 1
#endif

int WINAPI DownlevelGetLocaleScripts(LPCWSTR,LPWSTR,int);
int WINAPI DownlevelGetStringScripts(DWORD,LPCWSTR,int,LPWSTR,int);
BOOL WINAPI DownlevelVerifyScripts(DWORD,LPCWSTR,int,LPCWSTR,int);

#endif
