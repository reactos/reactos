/* $Id: main.c,v 1.2 2000/03/08 22:37:03 ea Exp $
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
