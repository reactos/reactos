/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Spool Files and printing
 * COPYRIGHT:   Copyright 1998-2020 ReactOS
 */

#include "precomp.h"


HANDLE WINAPI
GetSpoolFileHandle( HANDLE hPrinter )
{
    DWORD dwErrorCode, cpid;
    WINSPOOL_FILE_INFO_CONTAINER FileInfoContainer;
    WINSPOOL_FILE_INFO_1 wsplfi;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    HANDLE hHandle = NULL;

    FIXME("GetSpoolFileHandle(%p)\n", hPrinter);

    if ( IntProtectHandle( hPrinter, FALSE ) )
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }
    else
    {
        if ( pHandle->hSpoolFileHandle != INVALID_HANDLE_VALUE )
        {
              hHandle = pHandle->hSpoolFileHandle;
        }
        else
        {
            cpid = GetCurrentProcessId();

            FileInfoContainer.Level = 1;
            FileInfoContainer.FileInfo.pFileInfo1 = &wsplfi;

            // Do the RPC call.
            RpcTryExcept
            {
                dwErrorCode = _RpcGetSpoolFileInfo2( &pHandle->hPrinter, cpid, 1, &FileInfoContainer );
            }
            RpcExcept(EXCEPTION_EXECUTE_HANDLER)
            {
                dwErrorCode = RpcExceptionCode();
                ERR("_RpcGetSpoolFileInfo failed with exception code %lu!\n", dwErrorCode);
            }
            RpcEndExcept;

            if (dwErrorCode == ERROR_SUCCESS)
            {
                pHandle->hSpoolFileHandle = wsplfi.hSpoolFileHandle;
                pHandle->dwOptions        = wsplfi.dwOptions;
                hHandle                   = pHandle->hSpoolFileHandle;
            }
        }
        IntUnprotectHandle(pHandle);
    }
    SetLastError(dwErrorCode);
    return hHandle;
}

HANDLE WINAPI
CommitSpoolData( HANDLE hPrinter, HANDLE hSpoolFile, DWORD cbCommit )
{
    DWORD dwErrorCode, cpid;
    WINSPOOL_FILE_INFO_CONTAINER FileInfoContainer;
    WINSPOOL_FILE_INFO_1 wsplfi;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    FIXME("CommitSpoolData(%p, %p, %d)\n", hPrinter,hSpoolFile,cbCommit);

    if ( IntProtectHandle( hPrinter, FALSE ) )
    {
        return hHandle;
    }

    if ( pHandle->hSpoolFileHandle == INVALID_HANDLE_VALUE || pHandle->hSpoolFileHandle != hSpoolFile )
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }
    else
    {
        cpid = GetCurrentProcessId();

        FileInfoContainer.Level = 1;
        FileInfoContainer.FileInfo.pFileInfo1 = &wsplfi;

        // Do the RPC call.
        RpcTryExcept
        {
            dwErrorCode = _RpcCommitSpoolData2( &pHandle->hPrinter, cpid, cbCommit, 1, &FileInfoContainer );
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcCommitSpoolData failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;

        if (dwErrorCode == ERROR_SUCCESS)
        {
            if ( wsplfi.hSpoolFileHandle != INVALID_HANDLE_VALUE )
            {
                CloseHandle( pHandle->hSpoolFileHandle );
                pHandle->hSpoolFileHandle = wsplfi.hSpoolFileHandle;
            }
            hHandle = pHandle->hSpoolFileHandle;
        }
        IntUnprotectHandle(pHandle);
    }
    SetLastError(dwErrorCode);
    return hHandle;
}

BOOL WINAPI
CloseSpoolFileHandle( HANDLE hPrinter, HANDLE hSpoolFile )
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    FIXME("CloseSpoolFileHandle(%p, %p)\n", hPrinter,hSpoolFile);

    if ( IntProtectHandle( hPrinter, FALSE ) )
    {
        return FALSE;
    }
    if ( pHandle->hSpoolFileHandle == hSpoolFile )
    {
        CloseHandle( pHandle->hSpoolFileHandle );
        pHandle->hSpoolFileHandle = INVALID_HANDLE_VALUE;

        // Do the RPC call.
        RpcTryExcept
        {
            dwErrorCode = _RpcCloseSpoolFileHandle( &pHandle->hPrinter );
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            dwErrorCode = RpcExceptionCode();
            ERR("_RpcloseSpoolFileHandle failed with exception code %lu!\n", dwErrorCode);
        }
        RpcEndExcept;
    }
    else
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
    }
    IntUnprotectHandle(pHandle);
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
