/* $Id: main.c,v 1.1 1999/11/20 20:36:46 ea Exp $ */
#include <ntos.h>

BOOLEAN
STDCALL
DllMain (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
{
	return TRUE;
}


/* EOF */
