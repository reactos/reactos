#ifndef _INC_WIN32ERR_H
#define _INC_WIN32ERR_H
/* $Id: win32err.h,v 1.1 1999/05/16 07:27:35 ea Exp $ */
#ifndef _GNU_H_WINDOWS_H
#include <windows.h>
#endif /* ndef _GNU_H_WINDOWS_H */
VOID
PrintWin32Error(
	PWCHAR	Message,
	DWORD	ErrorCode
	);
#endif /* ndef _INC_WIN32ERR_H */
