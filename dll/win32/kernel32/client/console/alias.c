/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/alias.c
 * PURPOSE:         Win32 Console Client Alias support functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net) (welch@mcmail.com)
 *                  Christoph von Wittich (christoph_vw@reactos.org)
 *                  Johannes Anderwald (janderwald@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
AddConsoleAliasW(LPCWSTR lpSource,
                 LPCWSTR lpTarget,
                 LPCWSTR lpExeName)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_CONSOLE_ALIAS ConsoleAlias = &ApiMessage.Data.ConsoleAlias;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG CapturedStrings;

    DPRINT("AddConsoleAliasW enterd with lpSource %S lpTarget %S lpExeName %S\n", lpSource, lpTarget, lpExeName);

    /* Determine the needed sizes */
    ConsoleAlias->SourceLength = (wcslen(lpSource ) + 1) * sizeof(WCHAR);
    ConsoleAlias->ExeLength    = (wcslen(lpExeName) + 1) * sizeof(WCHAR);
    CapturedStrings = 2;

    if (lpTarget) /* The target can be optional */
    {
        ConsoleAlias->TargetLength = (wcslen(lpTarget) + 1) * sizeof(WCHAR);
        CapturedStrings++;
    }
    else
    {
        ConsoleAlias->TargetLength = 0;
    }

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(CapturedStrings,
                                             ConsoleAlias->SourceLength +
                                             ConsoleAlias->ExeLength    +
                                             ConsoleAlias->TargetLength);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Capture the strings */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpSource,
                            ConsoleAlias->SourceLength,
                            &ConsoleAlias->Source);

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpExeName,
                            ConsoleAlias->ExeLength,
                            &ConsoleAlias->Exe);

    if (lpTarget) /* The target can be optional */
    {
        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)lpTarget,
                                ConsoleAlias->TargetLength,
                                &ConsoleAlias->Target);
    }
    else
    {
        ConsoleAlias->Target = NULL;
    }

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAddAlias),
                                 sizeof(CSRSS_CONSOLE_ALIAS));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

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
        BasepAnsiStringToHeapUnicodeString(lpSource, (LPWSTR*)&lpSourceW);
    if (lpTarget)
        BasepAnsiStringToHeapUnicodeString(lpTarget, (LPWSTR*)&lpTargetW);
    if (lpExeName)
        BasepAnsiStringToHeapUnicodeString(lpExeName, (LPWSTR*)&lpExeNameW);

    bRetVal = AddConsoleAliasW(lpSourceW, lpTargetW, lpExeNameW);

    /* Clean up */
    if (lpSourceW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*)lpSourceW);
    if (lpTargetW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*)lpTargetW);
    if (lpExeNameW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*)lpExeNameW);

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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_CONSOLE_ALIAS ConsoleAlias = &ApiMessage.Data.ConsoleAlias;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    DPRINT("GetConsoleAliasW entered with lpSource %S lpExeName %S\n", lpSource, lpExeName);

    if (lpTargetBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Determine the needed sizes */
    ConsoleAlias->SourceLength = (wcslen(lpSource ) + 1) * sizeof(WCHAR);
    ConsoleAlias->ExeLength    = (wcslen(lpExeName) + 1) * sizeof(WCHAR);

    ConsoleAlias->Target = NULL;
    ConsoleAlias->TargetLength = TargetBufferLength;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(3, ConsoleAlias->SourceLength +
                                                ConsoleAlias->ExeLength    +
                                                ConsoleAlias->TargetLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Capture the strings */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpSource,
                            ConsoleAlias->SourceLength,
                            &ConsoleAlias->Source);

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpExeName,
                            ConsoleAlias->ExeLength,
                            &ConsoleAlias->Exe);

    /* Allocate space for the target buffer */
    CsrAllocateMessagePointer(CaptureBuffer,
                              ConsoleAlias->TargetLength,
                              (PVOID*)&ConsoleAlias->Target);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAlias),
                                 sizeof(CSRSS_CONSOLE_ALIAS));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Copy the returned target string into the user buffer */
    // wcscpy(lpTargetBuffer, ConsoleAlias->Target);
    memcpy(lpTargetBuffer,
           ConsoleAlias->Target,
           ConsoleAlias->TargetLength);

    /* Release the capture buffer and exits */
    CsrFreeCaptureBuffer(CaptureBuffer);

    return ConsoleAlias->TargetLength;
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
GetConsoleAliasesW(LPWSTR AliasBuffer,
                   DWORD AliasBufferLength,
                   LPWSTR ExeName)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_GET_ALL_CONSOLE_ALIASES GetAllConsoleAliases = &ApiMessage.Data.GetAllConsoleAliases;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    DPRINT("GetConsoleAliasesW entered\n");

    /* Determine the needed sizes */
    GetAllConsoleAliases->ExeLength = GetConsoleAliasesLengthW(ExeName);
    if (GetAllConsoleAliases->ExeLength == 0 ||
        GetAllConsoleAliases->ExeLength > AliasBufferLength)
    {
        return 0;
    }

    GetAllConsoleAliases->AliasesBufferLength = AliasBufferLength;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(2, GetAllConsoleAliases->ExeLength +
                                                GetAllConsoleAliases->AliasesBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Capture the exe name and allocate space for the aliases buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)ExeName,
                            GetAllConsoleAliases->ExeLength,
                            (PVOID*)&GetAllConsoleAliases->ExeName);

    CsrAllocateMessagePointer(CaptureBuffer,
                              GetAllConsoleAliases->AliasesBufferLength,
                              (PVOID*)&GetAllConsoleAliases->AliasesBuffer);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliases),
                                 sizeof(CSRSS_GET_ALL_CONSOLE_ALIASES));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Copy the returned aliases string into the user buffer */
    // wcscpy(AliasBuffer, GetAllConsoleAliases->AliasesBuffer);
    memcpy(AliasBuffer,
           GetAllConsoleAliases->AliasesBuffer,
           GetAllConsoleAliases->AliasesBufferLength);

    /* Release the capture buffer and exits */
    CsrFreeCaptureBuffer(CaptureBuffer);

    return GetAllConsoleAliases->AliasesBufferLength; // / sizeof(WCHAR); (original code)
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
        BasepAnsiStringToHeapUnicodeString(ExeName, (LPWSTR*)&lpwExeName);

    lpwAliasBuffer = HeapAlloc(GetProcessHeap(), 0, AliasBufferLength * sizeof(WCHAR));

    dwRetVal = GetConsoleAliasesW(lpwAliasBuffer, AliasBufferLength * sizeof(WCHAR), lpwExeName);

    if (lpwExeName)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*)lpwExeName);

    if (dwRetVal)
        dwRetVal = WideCharToMultiByte(CP_ACP, 0, lpwAliasBuffer, dwRetVal /**/ / sizeof(WCHAR) /**/, AliasBuffer, AliasBufferLength, NULL, NULL);

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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH GetAllConsoleAliasesLength = &ApiMessage.Data.GetAllConsoleAliasesLength;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    DPRINT("GetConsoleAliasesLengthW entered\n");

    if (lpExeName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    GetAllConsoleAliasesLength->ExeLength = (wcslen(lpExeName) + 1) * sizeof(WCHAR);
    GetAllConsoleAliasesLength->Length = 0;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, GetAllConsoleAliasesLength->ExeLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Capture the exe name */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpExeName,
                            GetAllConsoleAliasesLength->ExeLength,
                            (PVOID)&GetAllConsoleAliasesLength->ExeName);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasesLength),
                                 sizeof(CSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return GetAllConsoleAliasesLength->Length;
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
        BasepAnsiStringToHeapUnicodeString(lpExeName, (LPWSTR*)&lpExeNameW);

    dwRetVal = GetConsoleAliasesLengthW(lpExeNameW);
    if (dwRetVal)
        dwRetVal /= sizeof(WCHAR);

    /* Clean up */
    if (lpExeNameW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*)lpExeNameW);

    return dwRetVal;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleAliasExesW(LPWSTR lpExeNameBuffer,
                     DWORD ExeNameBufferLength)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_GET_CONSOLE_ALIASES_EXES GetConsoleAliasesExes = &ApiMessage.Data.GetConsoleAliasesExes;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    DPRINT("GetConsoleAliasExesW entered\n");

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, ExeNameBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    GetConsoleAliasesExes->Length = ExeNameBufferLength;

    /* Allocate space for the exe name buffer */
    CsrAllocateMessagePointer(CaptureBuffer,
                              ExeNameBufferLength,
                              (PVOID*)&GetConsoleAliasesExes->ExeNames);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasExes),
                                 sizeof(CSRSS_GET_CONSOLE_ALIASES_EXES));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Copy the returned target string into the user buffer */
    memcpy(lpExeNameBuffer,
           GetConsoleAliasesExes->ExeNames,
           GetConsoleAliasesExes->Length);

    /* Release the capture buffer and exits */
    CsrFreeCaptureBuffer(CaptureBuffer);

    return GetConsoleAliasesExes->Length;
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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH GetConsoleAliasesExesLength = &ApiMessage.Data.GetConsoleAliasesExesLength;

    DPRINT("GetConsoleAliasExesLengthW entered\n");

    GetConsoleAliasesExesLength->Length = 0;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasExesLength),
                                 sizeof(CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return GetConsoleAliasesExesLength->Length;
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

/* EOF */
