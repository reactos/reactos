/* $Id: init.c,v 1.2 1999/07/17 23:10:29 ea Exp $
 *
 * reactos/services/winlogon/init.c
 *
 */
#include <windows.h>

BOOL
Initialize(VOID)
{
	/* SERVICES CONTROLLER */
	NtCreateProcess(
		L"\\\\??\\C:\\reactos\\system\\services.exe"
		);
	/* LOCAL SECURITY AUTORITY SUBSYSTEM */
	NtCreateProcess(
		L"\\\\??\\C:\\reactos\\system\\lsass.exe"
		);
	return TRUE;
}


/* EOF */
