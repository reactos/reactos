/* $Id: main.c,v 1.3 2002/09/07 15:12:07 chorns Exp $
 *
 * DESCRIPTION: Entry point for TDI.SYS
 */
#include <ddk/ntddk.h>

NTSTATUS
STDCALL
DriverEntry (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
{
	return STATUS_UNSUCCESSFUL;
}



/* EOF */
