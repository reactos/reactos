/* $Id: main.c,v 1.1 1999/11/20 20:41:29 ea Exp $
 *
 * DESCRIPTION: Entry point for TDI.SYS
 */
#include <ntos.h>

BOOLEAN
STDCALL
DllMain (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
{
	return FALSE; /* ? */
}


/* EOF */
