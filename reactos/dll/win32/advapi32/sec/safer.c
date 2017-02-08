/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Safer functions
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *  SaferCreateLevel
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferCreateLevel(
    _In_ DWORD dwScopeId,
    _In_ DWORD dwLevelId,
    _In_ DWORD OpenFlags,
    _Outptr_ SAFER_LEVEL_HANDLE *pLevelHandle,
    _Reserved_ PVOID pReserved)
{
    FIXME("(%lu, %lu, %lu, %p, %p) stub\n", dwScopeId, dwLevelId, OpenFlags, pLevelHandle, pReserved);
    *pLevelHandle = (SAFER_LEVEL_HANDLE)0x42;
    return TRUE;
}


/**********************************************************************
 *  SaferIdentifyLevel
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferIdentifyLevel(
    _In_ DWORD dwNumProperties,
    _In_reads_opt_(dwNumProperties) PSAFER_CODE_PROPERTIES pCodeProperties,
    _Outptr_ SAFER_LEVEL_HANDLE *pLevelHandle,
    _Reserved_ PVOID pReserved)
{
    DWORD i;

    if (pLevelHandle == NULL)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    for (i = 0; i < dwNumProperties; i++)
    {
        if (pCodeProperties[i].cbSize != sizeof(SAFER_CODE_PROPERTIES_V1))
        {
            SetLastError(ERROR_BAD_LENGTH);
            return FALSE;
        }
    }

    FIXME("(%lu, %p, %p, %p) stub\n", dwNumProperties, pCodeProperties, pLevelHandle, pReserved);

    *pLevelHandle = (SAFER_LEVEL_HANDLE)0x42;
    return TRUE;
}


/**********************************************************************
 *  SaferCloseLevel
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferCloseLevel(
    _In_ SAFER_LEVEL_HANDLE hLevelHandle)
{
    FIXME("(%p) stub\n", hLevelHandle);
    if (hLevelHandle != (SAFER_LEVEL_HANDLE)0x42)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return TRUE;
}


BOOL
WINAPI
SaferGetLevelInformation(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_ SAFER_OBJECT_INFO_CLASS dwInfoType,
    _Out_writes_bytes_opt_(dwInBufferSize) PVOID pQueryBuffer,
    _In_ DWORD dwInBufferSize,
    _Out_ PDWORD pdwOutBufferSize);


BOOL
WINAPI
SaferSetLevelInformation(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_ SAFER_OBJECT_INFO_CLASS dwInfoType,
    _In_reads_bytes_(dwInBufferSize) PVOID pQueryBuffer,
    _In_ DWORD dwInBufferSize);


/**********************************************************************
 *  SaferGetPolicyInformation
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferGetPolicyInformation(
    _In_ DWORD dwScopeId,
    _In_ SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    _In_ DWORD InfoBufferSize,
    _Out_writes_bytes_opt_(InfoBufferSize) PVOID InfoBuffer,
    _Out_ PDWORD InfoBufferRetSize,
    _Reserved_ PVOID pReserved)
{
    FIXME("(%lu, %d, %lu, %p, %p, %p) stub\n", dwScopeId, SaferPolicyInfoClass, InfoBufferSize, InfoBuffer, InfoBufferRetSize, pReserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
SaferSetPolicyInformation(
    _In_ DWORD dwScopeId,
    _In_ SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    _In_ DWORD InfoBufferSize,
    _In_reads_bytes_(InfoBufferSize) PVOID InfoBuffer,
    _Reserved_ PVOID pReserved);


/**********************************************************************
 *  SaferComputeTokenFromLevel
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferComputeTokenFromLevel(
    _In_ SAFER_LEVEL_HANDLE LevelHandle,
    _In_opt_ HANDLE InAccessToken,
    _Out_ PHANDLE OutAccessToken,
    _In_ DWORD dwFlags,
    _Inout_opt_ PVOID pReserved)
{
    FIXME("(%p, %p, %p, 0x%lx, %p) stub\n", LevelHandle, InAccessToken, OutAccessToken, dwFlags, pReserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  SaferRecordEventLogEntry
 *
 * @unimplemented
 */
BOOL
WINAPI
SaferRecordEventLogEntry(
    _In_ SAFER_LEVEL_HANDLE hLevel,
    _In_ PCWSTR szTargetPath,
    _Reserved_ PVOID pReserved)
{
    FIXME("(%p, %s, %p) stub\n", hLevel, wine_dbgstr_w(szTargetPath), pReserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
SaferiIsExecutableFileType(
    _In_ PCWSTR szFullPath,
    _In_ BOOLEAN bFromShellExecute);

/* EOF */
