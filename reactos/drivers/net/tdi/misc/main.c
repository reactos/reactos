/* $Id: main.c,v 1.4 2002/09/08 10:22:19 chorns Exp $
 *
 * DESCRIPTION: Entry point for TDI.SYS
 */
#include <ntos.h>

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
