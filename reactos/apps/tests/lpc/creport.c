/* $Id: creport.c,v 1.1 1999/07/04 22:04:31 ea Exp $
 *
 * reactos/apps/lpc/creport.c
 *
 * To be run in a real WNT 4.0 system to
 * create an LPC named port.
 * 
 * Use Russinovich' HandleEx to verify
 * creport.exe owns the named LPC port
 * you asked to create.
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
(STDCALL * CreatePort)(
	/*OUT	PHANDLE			PortHandle,*/
	PVOID	Buffer,
	IN	POBJECT_ATTRIBUTES	PortAttributes	OPTIONAL,  
	IN	ACCESS_MASK		DesiredAccess,
	IN	DWORD			Unknown3,
	IN	ULONG			Flags
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
#define MAXARG   5000000


VOID
TryCreatePort(char *port_name)
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
		& PortName;
	ObjectAttributes.Attributes =
		0; //OBJ_CASE_INSENSITIVE --> STATUS_INVALID_PARAMETER ==> case sensitive!;
	ObjectAttributes.SecurityDescriptor =
		NULL;
	ObjectAttributes.SecurityQualityOfService =
		NULL;
	/*
	 * Try to issue a connection request.
	 */
	Port = 0;
	Status = CreatePort(
			& Port,
			& ObjectAttributes,
			0, /* ACCESS_MASK? */
			0, /* Unknown3 */
			LPC_CONNECT_FLAG5
			);
	if (Status == STATUS_SUCCESS)
	{
		DumpInfo(
			port_name_save,
			Status,
			"created",
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
		"Creating port \"%s\" failed (Status = %08X).\n",
		port_name_save,
		Status
		);
}


main( int argc, char * argv[] )
{
	HINSTANCE ntdll;

	if (argc != 2)
	{
		printf("WNT LPC Port Creator\n");
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
	printf("GetProcAddress(NTDLL.NtCreatePort)\n");
	CreatePort = (VOID*) GetProcAddress(
					ntdll,
					"NtCreatePort"
					);
	if (CreatePort == NULL)
	{
		FreeLibrary(ntdll);
		printf("Could not find NTDLL.NtCreatePort\n");
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
	printf("TryCreatePort(%s)\n",argv[1]);
	TryCreatePort(argv[1]);
	printf("Done\n");
	return EXIT_SUCCESS;
}

/* EOF */
