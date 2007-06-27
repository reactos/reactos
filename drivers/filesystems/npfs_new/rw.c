/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/rw.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#ifndef NDEBUG
VOID HexDump(PUCHAR Buffer, ULONG Length)
{
	CHAR Line[65];
	UCHAR ch;
	const char Hex[] = "0123456789ABCDEF";
	int i, j;

	DbgPrint("---------------\n");

	for (i = 0; i < Length; i+= 16)
	{
		memset(Line, ' ', 64);
		Line[64] = 0;

		for (j = 0; j < 16 && j + i < Length; j++)
		{
			ch = Buffer[i + j];
			Line[3*j + 0] = Hex[ch >> 4];
			Line[3*j + 1] = Hex[ch & 0x0f];
			Line[48 + j] = isprint(ch) ? ch : '.';
		}
		DbgPrint("%s\n", Line);
	}
	DbgPrint("---------------\n");
}
#endif

NTSTATUS NTAPI
NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsRead()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsWrite()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsFlushBuffers()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
