/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMERS:     Christoph von Wittich (christoph_vw@reactos.org)
 *                  Johannes Anderwald (janderwald@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @unimplemented
 */
BOOL
WINAPI
AddConsoleAliasW(LPCWSTR lpSource,
                 LPCWSTR lpTarget,
                 LPCWSTR lpExeName)
{
    PCSR_API_MESSAGE Request;
    NTSTATUS Status;
    ULONG SourceLength;
    ULONG TargetLength = 0;
    ULONG ExeLength;
    ULONG Size;
    ULONG RequestLength;
    WCHAR * Ptr;

    DPRINT("AddConsoleAliasW enterd with lpSource %S lpTarget %S lpExeName %S\n", lpSource, lpTarget, lpExeName);

    ExeLength = wcslen(lpExeName) + 1;
    SourceLength = wcslen(lpSource)+ 1;
    if (lpTarget)
        TargetLength = wcslen(lpTarget) + 1;

    Size = (ExeLength + SourceLength + TargetLength) * sizeof(WCHAR);
    RequestLength = sizeof(CSR_API_MESSAGE) + Size;

    Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, RequestLength);
    Ptr = (WCHAR*)(((ULONG_PTR)Request) + sizeof(CSR_API_MESSAGE));

    wcscpy(Ptr, lpSource);
    Request->Data.AddConsoleAlias.SourceLength = SourceLength;
    Ptr = (WCHAR*)(((ULONG_PTR)Request) + sizeof(CSR_API_MESSAGE) + SourceLength * sizeof(WCHAR));

    wcscpy(Ptr, lpExeName);
    Request->Data.AddConsoleAlias.ExeLength = ExeLength;
    Ptr = (WCHAR*)(((ULONG_PTR)Request) + sizeof(CSR_API_MESSAGE) + (ExeLength + SourceLength)* sizeof(WCHAR));

    if (lpTarget) /* target can be optional */
        wcscpy(Ptr, lpTarget);

    Request->Data.AddConsoleAlias.TargetLength = TargetLength;

    Status = CsrClientCallServer(Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, ADD_CONSOLE_ALIAS),
                                 RequestLength);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request->Status))
    {
        BaseSetLastNTError(Status);
        RtlFreeHeap(GetProcessHeap(), 0, Request);
        return FALSE;
    }

    RtlFreeHeap(GetProcessHeap(), 0, Request);
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
AddConsoleAliasA(LPCSTR lpSource,
                 LPCSTR lpTarget,
                 LPCSTR lpExeName)
{
    LPWSTR lpSourceW = NULL;
    LPWSTR lpTargetW = NULL;
    LPWSTR lpExeNameW = NULL;
    BOOL bRetVal;

    if (lpSource)
        BasepAnsiStringToHeapUnicodeString(lpSource, (LPWSTR*) &lpSourceW);
    if (lpTarget)
        BasepAnsiStringToHeapUnicodeString(lpTarget, (LPWSTR*) &lpTargetW);
    if (lpExeName)
        BasepAnsiStringToHeapUnicodeString(lpExeName, (LPWSTR*) &lpExeNameW);

    bRetVal = AddConsoleAliasW(lpSourceW, lpTargetW, lpExeNameW);

    /* Clean up */
    if (lpSourceW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpSourceW);
    if (lpTargetW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpTargetW);
    if (lpExeNameW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpExeNameW);

    return bRetVal;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasW(LPWSTR lpSource,
                 LPWSTR lpTargetBuffer,
                 DWORD TargetBufferLength,
                 LPWSTR lpExeName)
{
    PCSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;
    ULONG Size;
    ULONG ExeLength;
    ULONG SourceLength;
    ULONG RequestLength;
    WCHAR * Ptr;

    DPRINT("GetConsoleAliasW entered lpSource %S lpExeName %S\n", lpSource, lpExeName);

    if (lpTargetBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ExeLength = wcslen(lpExeName) + 1;
    SourceLength = wcslen(lpSource) + 1;

    Size = (ExeLength + SourceLength) * sizeof(WCHAR);

    RequestLength = Size + sizeof(CSR_API_MESSAGE);
    Request = RtlAllocateHeap(GetProcessHeap(), 0, RequestLength);
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, TargetBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        RtlFreeHeap(GetProcessHeap(), 0, Request);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    Request->Data.GetConsoleAlias.TargetBuffer = NULL;

    CsrCaptureMessageBuffer(CaptureBuffer,
                            NULL,
                            TargetBufferLength,
                            (PVOID*)&Request->Data.GetConsoleAlias.TargetBuffer);

    Request->Data.GetConsoleAlias.TargetBufferLength = TargetBufferLength;

    Ptr = (LPWSTR)((ULONG_PTR)Request + sizeof(CSR_API_MESSAGE));
    wcscpy(Ptr, lpSource);
    Ptr += SourceLength;
    wcscpy(Ptr, lpExeName);

    Request->Data.GetConsoleAlias.ExeLength = ExeLength;
    Request->Data.GetConsoleAlias.SourceLength = SourceLength;

    Status = CsrClientCallServer(Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_CONSOLE_ALIAS),
                                 sizeof(CSR_API_MESSAGE) + Size);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request->Status))
    {
        RtlFreeHeap(GetProcessHeap(), 0, Request);
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    wcscpy(lpTargetBuffer, Request->Data.GetConsoleAlias.TargetBuffer);
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrFreeCaptureBuffer(CaptureBuffer);

    return Request->Data.GetConsoleAlias.BytesWritten;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasA(LPSTR lpSource,
                 LPSTR lpTargetBuffer,
                 DWORD TargetBufferLength,
                 LPSTR lpExeName)
{
    LPWSTR lpwSource;
    LPWSTR lpwExeName;
    LPWSTR lpwTargetBuffer;
    UINT dwSourceSize;
    UINT dwExeNameSize;
    UINT dwResult;

    DPRINT("GetConsoleAliasA entered\n");

    if (lpTargetBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    dwSourceSize = (strlen(lpSource)+1) * sizeof(WCHAR);
    lpwSource = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSourceSize);
    if (lpwSource == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, lpSource, -1, lpwSource, dwSourceSize);

    dwExeNameSize = (strlen(lpExeName)+1) * sizeof(WCHAR);
    lpwExeName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwExeNameSize);
    if (lpwExeName == NULL)
    {
        HeapFree(GetProcessHeap(), 0, lpwSource);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, lpExeName, -1, lpwExeName, dwExeNameSize);

    lpwTargetBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, TargetBufferLength * sizeof(WCHAR));
    if (lpwTargetBuffer == NULL)
    {
        HeapFree(GetProcessHeap(), 0, lpwSource);
        HeapFree(GetProcessHeap(), 0, lpwExeName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    dwResult = GetConsoleAliasW(lpwSource, lpwTargetBuffer, TargetBufferLength * sizeof(WCHAR), lpwExeName);

    HeapFree(GetProcessHeap(), 0, lpwSource);
    HeapFree(GetProcessHeap(), 0, lpwExeName);

    if (dwResult)
        dwResult = WideCharToMultiByte(CP_ACP, 0, lpwTargetBuffer, dwResult / sizeof(WCHAR), lpTargetBuffer, TargetBufferLength, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, lpwTargetBuffer);

    return dwResult;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasExesW(LPWSTR lpExeNameBuffer,
                     DWORD ExeNameBufferLength)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status;

    DPRINT("GetConsoleAliasExesW entered\n");

    CaptureBuffer = CsrAllocateCaptureBuffer(1, ExeNameBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              ExeNameBufferLength,
                              (PVOID*)&Request.Data.GetConsoleAliasesExes.ExeNames);
    Request.Data.GetConsoleAliasesExes.Length = ExeNameBufferLength;

    Status = CsrClientCallServer(&Request,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_CONSOLE_ALIASES_EXES),
                                 sizeof(CSR_API_MESSAGE));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        CsrFreeCaptureBuffer(CaptureBuffer);
        return 0;
    }

    memcpy(lpExeNameBuffer,
           Request.Data.GetConsoleAliasesExes.ExeNames,
           Request.Data.GetConsoleAliasesExes.BytesWritten);

    CsrFreeCaptureBuffer(CaptureBuffer);
    return Request.Data.GetConsoleAliasesExes.BytesWritten;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasExesA(LPSTR lpExeNameBuffer,
                     DWORD ExeNameBufferLength)
{
    LPWSTR lpwExeNameBuffer;
    DWORD dwResult;

    DPRINT("GetConsoleAliasExesA entered\n");

    lpwExeNameBuffer = HeapAlloc(GetProcessHeap(), 0, ExeNameBufferLength * sizeof(WCHAR));

    dwResult = GetConsoleAliasExesW(lpwExeNameBuffer, ExeNameBufferLength * sizeof(WCHAR));

    if (dwResult)
        dwResult = WideCharToMultiByte(CP_ACP, 0, lpwExeNameBuffer, dwResult / sizeof(WCHAR), lpExeNameBuffer, ExeNameBufferLength, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, lpwExeNameBuffer);
    return dwResult;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasExesLengthW(VOID)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;

    DPRINT("GetConsoleAliasExesLengthW entered\n");

    Request.Data.GetConsoleAliasesExesLength.Length = 0;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_CONSOLE_ALIASES_EXES_LENGTH),
                                 sizeof(CSR_API_MESSAGE));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return Request.Data.GetConsoleAliasesExesLength.Length;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasExesLengthA(VOID)
{
    DWORD dwLength;

    DPRINT("GetConsoleAliasExesLengthA entered\n");

    dwLength = GetConsoleAliasExesLengthW();

    if (dwLength)
        dwLength /= sizeof(WCHAR);

    return dwLength;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasesW(LPWSTR AliasBuffer,
                   DWORD AliasBufferLength,
                   LPWSTR ExeName)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;
    DWORD dwLength;

    DPRINT("GetConsoleAliasesW entered\n");

    dwLength = GetConsoleAliasesLengthW(ExeName);
    if (!dwLength || dwLength > AliasBufferLength)
        return 0;

    Request.Data.GetAllConsoleAlias.AliasBuffer = AliasBuffer;
    Request.Data.GetAllConsoleAlias.AliasBufferLength = AliasBufferLength;
    Request.Data.GetAllConsoleAlias.lpExeName = ExeName;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_ALL_CONSOLE_ALIASES),
                                 sizeof(CSR_API_MESSAGE));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return Request.Data.GetAllConsoleAlias.BytesWritten / sizeof(WCHAR);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasesA(LPSTR AliasBuffer,
                   DWORD AliasBufferLength,
                   LPSTR ExeName)
{
    DWORD dwRetVal = 0;
    LPWSTR lpwExeName = NULL;
    LPWSTR lpwAliasBuffer;

    DPRINT("GetConsoleAliasesA entered\n");

    if (ExeName)
        BasepAnsiStringToHeapUnicodeString(ExeName, (LPWSTR*) &lpwExeName);

    lpwAliasBuffer = HeapAlloc(GetProcessHeap(), 0, AliasBufferLength * sizeof(WCHAR));

    dwRetVal = GetConsoleAliasesW(lpwAliasBuffer, AliasBufferLength, lpwExeName);

    if (lpwExeName)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpwExeName);

    if (dwRetVal)
        dwRetVal = WideCharToMultiByte(CP_ACP, 0, lpwAliasBuffer, dwRetVal, AliasBuffer, AliasBufferLength, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, lpwAliasBuffer);
    return dwRetVal;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasesLengthW(LPWSTR lpExeName)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;

    DPRINT("GetConsoleAliasesLengthW entered\n");

    Request.Data.GetAllConsoleAliasesLength.lpExeName = lpExeName;
    Request.Data.GetAllConsoleAliasesLength.Length = 0;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, GET_ALL_CONSOLE_ALIASES_LENGTH),
                                 sizeof(CSR_API_MESSAGE));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return Request.Data.GetAllConsoleAliasesLength.Length;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasesLengthA(LPSTR lpExeName)
{
    DWORD dwRetVal = 0;
    LPWSTR lpExeNameW = NULL;

    if (lpExeName)
        BasepAnsiStringToHeapUnicodeString(lpExeName, (LPWSTR*) &lpExeNameW);

    dwRetVal = GetConsoleAliasesLengthW(lpExeNameW);
    if (dwRetVal)
        dwRetVal /= sizeof(WCHAR);

    /* Clean up */
    if (lpExeNameW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpExeNameW);

    return dwRetVal;
}

/* EOF */
