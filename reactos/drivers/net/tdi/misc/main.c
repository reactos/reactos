/* $Id: main.c,v 1.5 2004/01/28 20:55:36 ekohl Exp $
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
