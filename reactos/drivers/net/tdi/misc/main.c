/* $Id$
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
