#ifndef _INCLUDE_KERNEL32_H
#define _INCLUDE_KERNEL32_H
/* $Id: error.h,v 1.1 2000/04/25 23:22:52 ea Exp $ */
#include <windows.h>
#define NTOS_MODE_USER
#include <ntos.h>
DWORD
STDCALL
SetLastErrorByStatus (
	NTSTATUS	Status
	);
#endif /* _INCLUDE_KERNEL32_H */
