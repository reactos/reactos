/* $Id: init.c,v 1.1 1999/06/18 22:40:47 ea Exp $
 *
 * resctos/services/winlogon/init.c
 *
 */
#include <windows.h>

BOOL
Initialize(VOID)
{
	/* SERVICES CONTROLLER */
	NtCreateProcess(
		L"\\\\??\\C:\reactos\system\services.exe"
		);
	/* LOCAL SECURITY AUTORITY SUBSYSTEM */
	NtCreateProcess(
		L"\\\\??\\C:\reactos\system\lsass.exe"
		);
	return TRUE;
}


/* EOF */
