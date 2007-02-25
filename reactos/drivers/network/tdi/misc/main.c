/* $Id$
 *
 * DESCRIPTION: Entry point for TDI.SYS
 */
#include <ntddk.h>

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
