/* $Id: conport.c,v 1.2 1999/06/27 07:11:25 ea Exp $
 *
 * reactos/apps/lpc/conport.c
 *
 * To be run in a real WNT 4.0 system with
 * "\SmApiPort" as argument. Do not try to
 * connect to "\Windows\ApiPort" since that
 * reboots immeditely.
 * 
 * Use Russinovich' HandleEx to verify
 * conport.exe owns two unnamed LPC ports:
 * the one created by kernel32.dll connecting
 * to csrss.exe, and one connected to here.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define PROTO_LPC
#include <ddk/ntddk.h>

#define LPC_CONNECT_FLAG1 0x00000001
#define LPC_CONNECT_FLAG2 0x00000010
#define LPC_CONNECT_FLAG3 0x00000100
#define LPC_CONNECT_FLAG4 0x00001000
#define LPC_CONNECT_FLAG5 0x00010000

#define QUERY_OBJECT_LPC_PORT_BASIC_INFORMATION_SIZE 55

NTSTATUS
(STDCALL * ConnectPort)(
	OUT	PHANDLE			PortHandle,
	IN	PUNICODE_STRING		PortName,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	DWORD	Unknown3,
	IN	DWORD	Unknown4,
	IN	DWORD	Unknown5,
	IN	DWORD	Unknown6,
	IN	ULONG	Flags
	);

NTSTATUS
(STDCALL * QueryObject)(
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	OUT	PVOID	ObjectInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	);
  
NTSTATUS
(STDCALL * YieldExecution)(VOID);

#define BUF_SIZE 1024
#define MAXARG   1000000

VOID
DumpBuffer(
	char	*Name,
	BYTE	*buffer,
	ULONG	size
	)
{
	register ULONG i = 0;

	printf("%s [%d] = ",size,Name);
	for (	i = 0;
		i != size;
		++i
		)
	{
		printf("%02X",buffer[i]);
	}
	printf("\n");
}


VOID
DumpInfo (
	LPCSTR		Name,
	NTSTATUS	Status,
	HANDLE		Port
	)
{
	int i;
	BYTE			ObjectInformation [BUF_SIZE] = {0};
	ULONG			ResultLength;

	printf("Port \"%s\" connected:\n",Name);

	printf("Status = %08X\n",Status);
	printf("Port   = %08X\n\n",Port);
	/*
	 * Query object information.
	 */
	printf("Basic Information:\n");
	Status = -1;
	for ( i=1024000; i && Status != STATUS_SUCCESS; --i)
	Status = QueryObject(
			Port,
			ObjectBasicInformation,
			ObjectInformation,
			QUERY_OBJECT_LPC_PORT_BASIC_INFORMATION_SIZE,
			& ResultLength
			);
	if (Status == STATUS_SUCCESS)
	{
		DumpBuffer(
			"RAW",
			ObjectInformation,
			ResultLength
			);
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
*/
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


VOID
TryConnectPort(char *port_name)
{
	DWORD			Status = 0;
	HANDLE			Port = 0;
	int			i;
	UNICODE_STRING		PortName;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	WORD			Name [BUF_SIZE] = {0};
	int			dwx = 0;

	/*
	 * Convert the port's name to Unicode.
	 */
	for (
		PortName.Length = 0;
		(	*port_name
			&& (PortName.Length < BUF_SIZE)
			);
		)
	{
		Name[PortName.Length++] = (WORD) *port_name++;
	}
	Name[PortName.Length] = 0;

	PortName.Length = PortName.Length * sizeof (WORD);
	PortName.MaximumLength = PortName.Length + sizeof (WORD);
	PortName.Buffer = (PWSTR) Name;
	/*
	 * Prepare the port object attributes.
	 */
	ObjectAttributes.Length =
		sizeof (OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory =
		NULL;
	ObjectAttributes.ObjectName =
		NULL /*& PortName */;
	ObjectAttributes.Attributes =
		OBJ_CASE_INSENSITIVE;
	ObjectAttributes.SecurityDescriptor =
		NULL;
	ObjectAttributes.SecurityQualityOfService =
		NULL;
	/*
	 * Try to issue a connection request.
	 */
	Port = 0;
	Status = ConnectPort(
			& Port,
			& PortName,
			& ObjectAttributes,
			0,
			0,
			0,
			0,
			LPC_CONNECT_FLAG5
			);
	if (Status == STATUS_SUCCESS)
	{
		DumpInfo(port_name,Status,Port);
		/* Hot waiting */
		for (dwx=0; dwx<MAXARG; ++dwx)
		{
			YieldExecution();
		}
		if (FALSE == CloseHandle(Port))
		{
			printf(
				"Could not close the port handle %08X.\n",
				Port
				);
		}
		return;
	}
	printf(
		"Connection to port \"%s\" failed (Status = %08X).\n",
		Status
		);
}


main( int argc, char * argv[] )
{
	HINSTANCE ntdll;

	if (argc != 2)
	{
		printf("WNT LPC Port Connector\n");
		printf("Usage: %s [port_name]\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	printf("LoadLibrary(NTDLL)\n");
	ntdll = LoadLibrary("NTDLL");
	if (ntdll == NULL)
	{
		printf("Could not load NTDLL\n");
		return EXIT_FAILURE;
	}
	printf("GetProcAddress(NTDLL.NtConnectPort)\n");
	ConnectPort = (VOID*) GetProcAddress(
					ntdll,
					"NtConnectPort"
					);
	if (ConnectPort == NULL)
	{
		FreeLibrary(ntdll);
		printf("Could not find NTDLL.NtConnectPort\n");
		return EXIT_FAILURE;
	}
	printf("GetProcAddress(NTDLL.NtQueryObject)\n");
	QueryObject = (VOID*) GetProcAddress(
					ntdll,
					"NtQueryObject"
					);
	if (QueryObject == NULL)
	{
		FreeLibrary(ntdll);
		printf("Could not find NTDLL.NtQueryObject\n");
		return EXIT_FAILURE;
	}
	printf("GetProcAddress(NTDLL.NtYieldExecution)\n");
	YieldExecution = (VOID*) GetProcAddress(
					ntdll,
					"NtYieldExecution"
					);
	if (YieldExecution == NULL)
	{
		FreeLibrary(ntdll);
		printf("Could not find NTDLL.NtYieldExecution\n");
		return EXIT_FAILURE;
	}
	printf("TryConnectPort(%s)\n",argv[1]);
	TryConnectPort(argv[1]);
	printf("Done\n");
	return EXIT_SUCCESS;
}

/* EOF */
