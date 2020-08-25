/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Spool File RPC calls
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

DWORD
_RpcGetSpoolFileInfo( WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_HANDLE hProcessHandle, DWORD Level, WINSPOOL_FILE_INFO_1* pFileInfo, DWORD dwSize, DWORD* dwNeeded )
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SplGetSpoolFileInfo( hPrinter, hProcessHandle, Level, (FILE_INFO_1*)pFileInfo, dwSize, dwNeeded ) )
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcCommitSpoolData( WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_HANDLE hProcessHandle, DWORD cbCommit, DWORD Level, WINSPOOL_FILE_INFO_1* pFileInfo, DWORD dwSize, DWORD* dwNeeded )
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SplCommitSpoolData( hPrinter, hProcessHandle, cbCommit, Level, (FILE_INFO_1*)pFileInfo, dwSize, dwNeeded ) )
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcGetSpoolFileInfo2( WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwProcessId, DWORD Level, WINSPOOL_FILE_INFO_CONTAINER* pFileInfoContainer )
{
    DWORD dwErrorCode, dwNeeded = 0;
    HANDLE hProcessHandle;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    hProcessHandle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwProcessId );


    if (!SplGetSpoolFileInfo( hPrinter, hProcessHandle, Level, (FILE_INFO_1*)pFileInfoContainer->FileInfo.pFileInfo1, sizeof(FILE_INFO_1), &dwNeeded ) )
        dwErrorCode = GetLastError();

    if ( hProcessHandle )
    {
        CloseHandle( hProcessHandle );
    }

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcCommitSpoolData2( WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwProcessId, DWORD cbCommit, DWORD Level, WINSPOOL_FILE_INFO_CONTAINER* pFileInfoContainer )
{
    DWORD dwErrorCode, dwNeeded = 0;
    HANDLE hProcessHandle;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    hProcessHandle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwProcessId );

    if (!SplCommitSpoolData( hPrinter, hProcessHandle, cbCommit, Level, (FILE_INFO_1*)pFileInfoContainer->FileInfo.pFileInfo1, sizeof(FILE_INFO_1), &dwNeeded ) )
        dwErrorCode = GetLastError();

    if ( hProcessHandle )
    {
        CloseHandle( hProcessHandle );
    }

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcCloseSpoolFileHandle( WINSPOOL_PRINTER_HANDLE hPrinter )
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SplCloseSpoolFileHandle( hPrinter ) )
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
