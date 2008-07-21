/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* DRIVER ENTRYPOINT *********************************************************/

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING	RegistryPath)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
