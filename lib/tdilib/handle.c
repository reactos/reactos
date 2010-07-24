/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI interface
 * FILE:        handle.c
 * PURPOSE:     TDI transport handle management
 */

#include "iphlpapi_private.h"

const PWCHAR TcpFileName = L"\\Device\\Tcp";

NTSTATUS openTcpFile(PHANDLE tcpFile)
{
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    RtlInitUnicodeString( &fileName, TcpFileName );

    InitializeObjectAttributes( &objectAttributes,
                                &fileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwCreateFile( tcpFile,
                           SYNCHRONIZE | GENERIC_EXECUTE |
                           GENERIC_READ | GENERIC_WRITE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           0,
                           0 );

    /* String does not need to be freed: it points to the constant
     * string we provided */

    if (!NT_SUCCESS(status))
        *tcpFile = INVALID_HANDLE_VALUE;

    return status;
}

VOID closeTcpFile( HANDLE h )
{
    ASSERT(h != INVALID_HANDLE_VALUE);

    NtClose( h );
}
