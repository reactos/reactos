#include <windef.h>
#define NDEBUG
#include <reactos/debug.h>

#define LOADPERF_FUNCTION DWORD WINAPI

LOADPERF_FUNCTION
BackupPerfRegistryToFileW(
    IN LPCWSTR szFileName,
    IN LPCWSTR szCommentString OPTIONAL)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
LoadMofFromInstalledServiceA(
    IN LPCSTR szServiceName,
    IN LPCSTR szMofFilename,
    IN ULONG_PTR dwFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
LoadMofFromInstalledServiceW(
    IN LPCWSTR szServiceName,
    IN LPCWSTR szMofFilename,
    IN ULONG_PTR dwFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
RestorePerfRegistryFromFileW(
    IN LPCWSTR szFileName,
    IN LPCWSTR szLangId)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
SetServiceAsTrustedA(
    IN  LPCSTR    szReserved,
    IN  LPCSTR    szServiceName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
SetServiceAsTrustedW(
    IN LPCWSTR szReserved OPTIONAL,
    IN LPCWSTR szServiceName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesA(
    IN LPCSTR szNewCtrFilePath,
    IN LPCSTR szNewHlpFilePath OPTIONAL,
    IN LPSTR szLanguageID,
    IN ULONG_PTR dwFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesW(
    IN LPCWSTR szNewCtrFilePath,
    IN LPCWSTR szNewHlpFilePath OPTIONAL,
    IN LPWSTR szLanguageID,
    IN ULONG_PTR dwFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}
