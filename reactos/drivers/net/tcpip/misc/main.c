/* $Id: main.c,v 1.2 2000/03/08 22:37:02 ea Exp $ */
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
