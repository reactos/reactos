/* $Id: init.c,v 1.3 2002/04/14 18:06:39 ea Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        reactos/subsys/psx/lib/psxdll/misc/init.c
 * PURPOSE:     Client initialization
 * PROGRAMMER:  Emanuele Aliberti
 * UPDATE HISTORY:
 *               2001-05-06
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <psx/lpcproto.h>

/* DLL GLOBALS */
int * errno = NULL;
char *** _environ = NULL;
HANDLE ApiPort = INVALID_HANDLE_VALUE;
/*
 * Called by startup code in crt0.o, where real
 * errno and _environ are actually defined.
 */
VOID STDCALL __PdxInitializeData (int * errno_arg, char *** environ_arg)
{
	errno    = errno_arg;	
	_environ = environ_arg;	
}
/*
 * Called by DLL's entry point when reason==PROCESS_ATTACH.
 */
NTSTATUS STDCALL PsxConnectApiPort (VOID)
{
    UNICODE_STRING              usApiPortName;
    LPWSTR                      wsApiPortName = L"\\"PSX_NS_SUBSYSTEM_DIRECTORY_NAME"\\"PSX_NS_API_PORT_NAME;
    SECURITY_QUALITY_OF_SERVICE Sqos;
    ULONG                       MaxMessageSize = 0;
    NTSTATUS                    Status;
    PSX_CONNECT_PORT_DATA       ConnectData;
    ULONG                       ConnectDataLength = sizeof ConnectData;

    RtlInitUnicodeString (& usApiPortName, wsApiPortName);
    RtlZeroMemory (& Sqos, sizeof Sqos);
    ConnectData.ConnectionType = PSX_CONNECTION_TYPE_PROCESS;
    ConnectData.Version        = PSX_LPC_PROTOCOL_VERSION;
    ConnectData.PortIdentifier = 0;
    Status = NtConnectPort (
                & ApiPort,
		& usApiPortName,
		& Sqos,
		NULL,
		NULL,
		& MaxMessageSize,
		& ConnectData,
		& ConnectDataLength
		);
    if (!NT_SUCCESS(Status))
    {
        /* TODO: emit a diagnostic message */
        return Status;
    }
    /* TODO: save returned data */
    return STATUS_SUCCESS;
}
/* EOF */

