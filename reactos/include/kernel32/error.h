#ifndef _INCLUDE_KERNEL32_H
#define _INCLUDE_KERNEL32_H
/* $Id: error.h,v 1.2 2002/09/07 15:12:16 chorns Exp $ */
#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
DWORD
STDCALL
SetLastErrorByStatus (
	NTSTATUS	Status
	);
#endif /* _INCLUDE_KERNEL32_H */
