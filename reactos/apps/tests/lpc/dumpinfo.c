/* $Id: dumpinfo.c,v 1.1 1999/07/04 22:04:31 ea Exp $
 *
 * reactos/apps/lpc/dumpinfo.c
 *
 * ReactOS Operating System
 *
 * Dump a kernel object's attributes by its handle.
 *
 * 19990627 (Emanuele Aliberti)
 * 	Initial implementation.
 * 19990704 (EA)
 * 	Added code to find the basic information buffer size
 * 	for the LPC port object.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ddk/ntddk.h>

#define BUF_SIZE		1024
#define MAX_BASIC_INFO_SIZE	512


extern
NTSTATUS
(STDCALL * QueryObject)(
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	OUT	PVOID	ObjectInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	);

/*
static
VOID
DumpBuffer(
	char	*Name,
	BYTE	*buffer,
	ULONG	size
	)
{
	register ULONG i = 0;

	printf("%s [%d] = ",Name,size);
	for (	i = 0;
		i != size;
		++i
		)
	{
		printf("%02X",buffer[i]);
	}
	printf("\n");
}
*/

VOID
DumpInfo (
	LPCSTR		Name,
	NTSTATUS	Status,
	LPCSTR		Comment,
	HANDLE		Port
	)
{
	BYTE			ObjectInformation [BUF_SIZE] = {0};
	ULONG			ResultLength;
	
	printf("Port \"%s\" %s:\n",Name,Comment);

	printf("\tStatus = %08X\n",Status);
	printf("\tPort   = %08X\n\n",Port);
	/*
	 * Query object information.
	 */
	printf("Basic Information:\n");
	Status = QueryObject(
			Port,
			ObjectBasicInformation,
			ObjectInformation,
			sizeof (LPC_PORT_BASIC_INFORMATION),
			& ResultLength
			);
	if (Status == STATUS_SUCCESS)
	{
		PLPC_PORT_BASIC_INFORMATION	i;

		i = (PLPC_PORT_BASIC_INFORMATION) ObjectInformation;
	
		printf( "\tUnknown01 = 0x%08X\n", i->Unknown0 );
		printf( "\tUnknown02 = 0x%08X\n", i->Unknown1 );
		printf( "\tUnknown03 = 0x%08X\n", i->Unknown2 );
		printf( "\tUnknown04 = 0x%08X\n", i->Unknown3 );
		printf( "\tUnknown05 = 0x%08X\n", i->Unknown4 );
		printf( "\tUnknown06 = 0x%08X\n", i->Unknown5 );
		printf( "\tUnknown07 = 0x%08X\n", i->Unknown6 );
		printf( "\tUnknown08 = 0x%08X\n", i->Unknown7 );
		printf( "\tUnknown09 = 0x%08X\n", i->Unknown8 );
		printf( "\tUnknown10 = 0x%08X\n", i->Unknown9 );
		printf( "\tUnknown11 = 0x%08X\n", i->Unknown10 );
		printf( "\tUnknown12 = 0x%08X\n", i->Unknown11 );
		printf( "\tUnknown13 = 0x%08X\n", i->Unknown12 );
		printf( "\tUnknown14 = 0x%08X\n", i->Unknown13 );
	}
	else
	{
		printf("\tStatus = %08X\n",Status);
	}
	printf("Type Information:\n");
	Status = QueryObject(
			Port,
			ObjectTypeInformation,
			ObjectInformation,
			sizeof ObjectInformation,
			& ResultLength
			);
	if (Status == STATUS_SUCCESS)
	{
		OBJECT_TYPE_INFORMATION	* i;

		i = (OBJECT_TYPE_INFORMATION *) ObjectInformation;
		
		wprintf(
			L"\tName: \"%s\"\n",
			(i->Name.Length ? i->Name.Buffer : L"")
			);
/*
FIXME: why this always raise an access violation exception?
		wprintf(
			L"\tType: \"%s\"\n",
			(i->Type.Length ? i->Type.Buffer : L"")
			);
/**/
		printf(
			"\tTotal Handles: %d\n",
			i->TotalHandles
			);
		printf(
			"\tReference Count: %d\n",
			i->ReferenceCount
			);
	}
	else
	{
		printf("\tStatus = %08X\n",Status);
	}
	printf("Name Information:\n");
	Status = QueryObject(
			Port,
			ObjectNameInformation,
			ObjectInformation,
			sizeof ObjectInformation,
			& ResultLength
			);
	if (Status == STATUS_SUCCESS)
	{
		OBJECT_NAME_INFORMATION	* i;

		i = (OBJECT_NAME_INFORMATION *) ObjectInformation;
		wprintf(
			L"\tName: \"%s\"\n",
			(i->Name.Length ? i->Name.Buffer : L"")
			);
	}
	else
	{
		printf("\tStatus = %08X\n",Status);
	}
	printf("Data Information:\n");
	Status = QueryObject(
			Port,
			ObjectDataInformation,
			ObjectInformation,
			sizeof ObjectInformation,
			& ResultLength
			);
	if (Status == STATUS_SUCCESS)
	{
		OBJECT_DATA_INFORMATION	* i;

		i = (OBJECT_DATA_INFORMATION *) ObjectInformation;
		printf(
			"\tInherit Handle: %s\n",
			(i->bInheritHandle ? "TRUE" : "FALSE")
			);
		printf(
			"\tProtect from Close: %s\n",
			(i->bProtectFromClose ? "TRUE" : "FALSE")
			);
	}
	else
	{
		printf("\tStatus = %08X\n",Status);
	}
}


/* EOF */
