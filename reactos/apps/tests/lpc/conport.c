/* $Id: conport.c,v 1.6 2000/04/25 23:22:46 ea Exp $
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
 *
 * 19990627 (Emanuele Aliberti)
 * 	Initial implementation.
 * 19990704 (EA)
 * 	Dump object's attributes moved in dumpinfo.c.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define PROTO_LPC
#include <ddk/ntddk.h>
#include "dumpinfo.h"

#define LPC_CONNECT_FLAG1 0x00000001
#define LPC_CONNECT_FLAG2 0x00000010
#define LPC_CONNECT_FLAG3 0x00000100
#define LPC_CONNECT_FLAG4 0x00001000
#define LPC_CONNECT_FLAG5 0x00010000

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
TryConnectPort(char *port_name)
{
	DWORD			Status = 0;
	HANDLE			Port = 0;
	int			i;
	UNICODE_STRING		PortName;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	WORD			Name [BUF_SIZE] = {0};
	int			dwx = 0;
	char			* port_name_save = port_name;

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
			& Port,			/* & PortHandle */
			& PortName,		/* & PortName */
			& ObjectAttributes,	/* & PortAttributes */
			NULL,			/* & SecurityQos */
			NULL,			/* & SectionInfo */
			NULL,			/* & MapInfo */
			NULL,			/* & MaxMessageSize */
			LPC_CONNECT_FLAG5	/* & ConnectInfoLength */
			);
	if (Status == STATUS_SUCCESS)
	{
		DumpInfo(
			Name,
			Status,
			"connected",
			Port
			);
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
		port_name_save,
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
