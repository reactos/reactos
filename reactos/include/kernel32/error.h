#ifndef _INCLUDE_KERNEL32_H
#define _INCLUDE_KERNEL32_H
/* $Id: error.h,v 1.3 2002/09/08 10:22:30 chorns Exp $ */
#include <windows.h>
#define NTOS_MODE_USER
#include <ntos.h>
DWORD
STDCALL
SetLastErrorByStatus (
	NTSTATUS	Status
	);
#endif /* _INCLUDE_KERNEL32_H */
