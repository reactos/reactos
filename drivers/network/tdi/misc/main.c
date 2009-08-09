/* $Id$
 *
 * DESCRIPTION: Entry point for TDI.SYS
 */
#include <ntddk.h>

NTSTATUS
NTAPI
DriverEntry (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
{
	return STATUS_UNSUCCESSFUL;
}



/* EOF */
