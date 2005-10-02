#if !defined(_NTRT0_H)
#define _NTRT0_H

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

extern int _cdecl _main(int,char**,char**,int);

NTSTATUS STDCALL NtRtParseCommandLine (PPEB,int*,char**);

#endif /* !def _NTRT0_H */
	
