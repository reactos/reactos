/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Boot Data implementation
 * FILE:            lib/rtl/bootdata.c
 * PROGRAMMERS:     
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlCreateSystemVolumeInformationFolder(
	IN PUNICODE_STRING VolumeRootPath
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlGetSetBootStatusData(
	HANDLE Filehandle,
	BOOLEAN WriteMode,
	DWORD DataClass,
	PVOID Buffer,
	ULONG BufferSize,
	DWORD DataClass2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlLockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlUnlockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
