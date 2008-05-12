/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/rw.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

#ifndef NDEBUG
VOID HexDump(IN PUCHAR Buffer,
             IN SIZE_T Length)
{
	CHAR Line[65];
	UCHAR ch;
	const char Hex[] = "0123456789ABCDEF";
	ULONG i, j;

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
NpfsRead(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsRead()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsWrite(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsWrite()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsFlushBuffers()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
