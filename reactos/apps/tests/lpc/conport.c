/* $Id: conport.c,v 1.1 1999/06/24 22:54:27 ea Exp $
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

NTSTATUS
(STDCALL * ConnectPort)(
	PHANDLE,
	PUNICODE_STRING,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD,
	DWORD
	);

NTSTATUS
(STDCALL * YieldExecution)(VOID);

#define BUF_SIZE 256

VOID
DumpBuffer(
	char	*Name,
	BYTE	*buffer
	)
{
	register int i = 0;

	printf("%s = ",Name);
	for (	i = 0;
		i < BUF_SIZE;
		++i
		)
	{
		printf("%02X",buffer[i]);
	}
	printf("\n");
}

#define MAXARG 1000000

VOID
TryConnectPort(char *port_name)
{
	DWORD		Status = 0;
	HANDLE		Port = 0;
	int		i;
	UNICODE_STRING	PortName;
	WORD		Name [BUF_SIZE] = {0};
	int		dwx = 0;
	BYTE		bb [BUF_SIZE] = {0};

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

	Port = 0;
	Status = ConnectPort(
			& Port,
			& PortName,
			bb,
			0,
			0,
			0,
			0,
			LPC_CONNECT_FLAG5
			);
	if (Port) 
	{
		printf("Status = %08X\n",Status);
		printf("Port   = %08X\n\n",Port);
		for (dwx=0; dwx<MAXARG; ++dwx)
		{
			YieldExecution();
		}
		CloseHandle(Port);
	}
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
	printf("GetProcAddress(NTDLL.NtYieldExecution)\n");
	YieldExecution = (VOID*) GetProcAddress(
					ntdll,
					"NtYieldExecution"
					);
	if (ConnectPort == NULL)
	{
		FreeLibrary(ntdll);
		printf("Could not find NTDLL.NtConnectPort\n");
		return EXIT_FAILURE;
	}
	printf("TryConnectPort(%s)\n",argv[1]);
	TryConnectPort(argv[1]);
	printf("Done\n");
	return EXIT_SUCCESS;
}
