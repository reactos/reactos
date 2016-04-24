/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI interface
 * FILE:        handle.c
 * PURPOSE:     TDI transport handle management
 */

#include "precomp.h"

const PWCHAR TcpFileName = L"\\Device\\Tcp";

NTSTATUS openTcpFile(PHANDLE tcpFile, ACCESS_MASK DesiredAccess)
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

    status = NtOpenFile( tcpFile,
                         DesiredAccess | SYNCHRONIZE,
                         &objectAttributes,
                         &ioStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

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
