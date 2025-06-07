/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/alias.c
 * PURPOSE:         Win32 Console Client Alias support functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net) (welch@mcmail.com)
 *                  Christoph von Wittich (christoph_vw@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static BOOL
IntAddConsoleAlias(LPCVOID Source,
                   USHORT SourceBufferLength,
                   LPCVOID Target,
                   USHORT TargetBufferLength,
                   LPCVOID ExeName,
                   BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &ApiMessage.Data.ConsoleAliasRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG CapturedStrings;

    USHORT NumChars = (USHORT)(ExeName ? (bUnicode ? wcslen(ExeName) : strlen(ExeName)) : 0);

    if (ExeName == NULL || NumChars == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ConsoleAliasRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    /* Determine the needed sizes */
    ConsoleAliasRequest->SourceLength = SourceBufferLength;
    ConsoleAliasRequest->ExeLength    = NumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    ConsoleAliasRequest->Unicode  =
    ConsoleAliasRequest->Unicode2 = bUnicode;

    CapturedStrings = 2;

    if (Target) /* The target can be optional */
    {
        ConsoleAliasRequest->TargetLength = TargetBufferLength;
        CapturedStrings++;
    }
    else
    {
        ConsoleAliasRequest->TargetLength = 0;
    }

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(CapturedStrings,
                                             ConsoleAliasRequest->SourceLength +
                                             ConsoleAliasRequest->ExeLength    +
                                             ConsoleAliasRequest->TargetLength);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Capture the strings */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)Source,
                            ConsoleAliasRequest->SourceLength,
                            (PVOID*)&ConsoleAliasRequest->Source);

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)ExeName,
                            ConsoleAliasRequest->ExeLength,
                            (PVOID*)&ConsoleAliasRequest->ExeName);

    if (Target) /* The target can be optional */
    {
        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)Target,
                                ConsoleAliasRequest->TargetLength,
                                (PVOID*)&ConsoleAliasRequest->Target);
    }
    else
    {
        ConsoleAliasRequest->Target = NULL;
    }

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAddAlias),
                        sizeof(*ConsoleAliasRequest));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
AddConsoleAliasW(
    _In_ LPCWSTR Source,
    _In_ LPCWSTR Target,
    _In_ LPCWSTR ExeName)
{
    USHORT SourceBufferLength = (USHORT)wcslen(Source) * sizeof(WCHAR);
    USHORT TargetBufferLength = (USHORT)(Target ? wcslen(Target) * sizeof(WCHAR) : 0);

    DPRINT("AddConsoleAliasW entered with Source '%S' Target '%S' ExeName '%S'\n",
           Source, Target, ExeName);

    return IntAddConsoleAlias(Source,
                              SourceBufferLength,
                              Target,
                              TargetBufferLength,
                              ExeName,
                              TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
AddConsoleAliasA(
    _In_ LPCSTR Source,
    _In_ LPCSTR Target,
    _In_ LPCSTR ExeName)
{
    USHORT SourceBufferLength = (USHORT)strlen(Source) * sizeof(CHAR);
    USHORT TargetBufferLength = (USHORT)(Target ? strlen(Target) * sizeof(CHAR) : 0);

    DPRINT("AddConsoleAliasA entered with Source '%s' Target '%s' ExeName '%s'\n",
           Source, Target, ExeName);

    return IntAddConsoleAlias(Source,
                              SourceBufferLength,
                              Target,
                              TargetBufferLength,
                              ExeName,
                              FALSE);
}


static DWORD
IntGetConsoleAlias(LPCVOID Source,
                   USHORT SourceBufferLength,
                   LPVOID Target,
                   USHORT TargetBufferLength,
                   LPCVOID ExeName,
                   BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ADDGETALIAS ConsoleAliasRequest = &ApiMessage.Data.ConsoleAliasRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    USHORT NumChars = (USHORT)(ExeName ? (bUnicode ? wcslen(ExeName) : strlen(ExeName)) : 0);

    if (Source == NULL || Target == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (ExeName == NULL || NumChars == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ConsoleAliasRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    /* Determine the needed sizes */
    ConsoleAliasRequest->SourceLength = SourceBufferLength;
    ConsoleAliasRequest->ExeLength    = NumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    ConsoleAliasRequest->Unicode  =
    ConsoleAliasRequest->Unicode2 = bUnicode;

    ConsoleAliasRequest->TargetLength = TargetBufferLength;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(3, ConsoleAliasRequest->SourceLength +
                                                ConsoleAliasRequest->ExeLength    +
                                                ConsoleAliasRequest->TargetLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Capture the strings */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)Source,
                            ConsoleAliasRequest->SourceLength,
                            (PVOID*)&ConsoleAliasRequest->Source);

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)ExeName,
                            ConsoleAliasRequest->ExeLength,
                            (PVOID*)&ConsoleAliasRequest->ExeName);

    /* Allocate space for the target buffer */
    CsrAllocateMessagePointer(CaptureBuffer,
                              ConsoleAliasRequest->TargetLength,
                              (PVOID*)&ConsoleAliasRequest->Target);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAlias),
                        sizeof(*ConsoleAliasRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(ApiMessage.Status);

        if (ApiMessage.Status == STATUS_BUFFER_TOO_SMALL)
            return ConsoleAliasRequest->TargetLength;
        else
            return 0;
    }

    /* Copy the returned target string into the user buffer */
    RtlCopyMemory(Target,
                  ConsoleAliasRequest->Target,
                  ConsoleAliasRequest->TargetLength);

    /* Release the capture buffer and exit */
    CsrFreeCaptureBuffer(CaptureBuffer);

    return ConsoleAliasRequest->TargetLength;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasW(
    _In_ LPCWSTR Source,
    _Out_writes_(TargetBufferLength) LPWSTR TargetBuffer,
    _In_ DWORD TargetBufferLength,
    _In_ LPCWSTR ExeName)
{
    DPRINT("GetConsoleAliasW entered with Source '%S' ExeName '%S'\n",
           Source, ExeName);

    return IntGetConsoleAlias(Source,
                              (USHORT)wcslen(Source) * sizeof(WCHAR),
                              TargetBuffer,
                              TargetBufferLength,
                              ExeName,
                              TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasA(
    _In_ LPCSTR Source,
    _Out_writes_(TargetBufferLength) LPSTR TargetBuffer,
    _In_ DWORD TargetBufferLength,
    _In_ LPCSTR ExeName)
{
    DPRINT("GetConsoleAliasA entered with Source '%s' ExeName '%s'\n",
           Source, ExeName);

    return IntGetConsoleAlias(Source,
                              (USHORT)strlen(Source) * sizeof(CHAR),
                              TargetBuffer,
                              TargetBufferLength,
                              ExeName,
                              FALSE);
}


static DWORD
IntGetConsoleAliases(LPVOID AliasBuffer,
                     DWORD AliasBufferLength,
                     LPCVOID ExeName,
                     BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETALLALIASES GetAllAliasesRequest = &ApiMessage.Data.GetAllAliasesRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    USHORT NumChars = (USHORT)(ExeName ? (bUnicode ? wcslen(ExeName) : strlen(ExeName)) : 0);

    if (ExeName == NULL || NumChars == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    GetAllAliasesRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    /* Determine the needed sizes */
    GetAllAliasesRequest->ExeLength = NumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    GetAllAliasesRequest->Unicode   =
    GetAllAliasesRequest->Unicode2  = bUnicode;

    GetAllAliasesRequest->AliasesBufferLength = AliasBufferLength;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(2, GetAllAliasesRequest->ExeLength +
                                                GetAllAliasesRequest->AliasesBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Capture the exe name and allocate space for the aliases buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)ExeName,
                            GetAllAliasesRequest->ExeLength,
                            (PVOID*)&GetAllAliasesRequest->ExeName);

    CsrAllocateMessagePointer(CaptureBuffer,
                              GetAllAliasesRequest->AliasesBufferLength,
                              (PVOID*)&GetAllAliasesRequest->AliasesBuffer);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliases),
                        sizeof(*GetAllAliasesRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    /* Copy the returned aliases string into the user buffer */
    RtlCopyMemory(AliasBuffer,
                  GetAllAliasesRequest->AliasesBuffer,
                  GetAllAliasesRequest->AliasesBufferLength);

    /* Release the capture buffer and exit */
    CsrFreeCaptureBuffer(CaptureBuffer);

    return GetAllAliasesRequest->AliasesBufferLength;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasesW(
    _Out_writes_(AliasBufferLength) LPWSTR AliasBuffer,
    _In_ DWORD AliasBufferLength,
    _In_ LPCWSTR ExeName)
{
    DPRINT("GetConsoleAliasesW entered with ExeName '%S'\n",
            ExeName);

    return IntGetConsoleAliases(AliasBuffer,
                                AliasBufferLength,
                                ExeName,
                                TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasesA(
    _Out_writes_(AliasBufferLength) LPSTR AliasBuffer,
    _In_ DWORD AliasBufferLength,
    _In_ LPCSTR ExeName)
{
    DPRINT("GetConsoleAliasesA entered with ExeName '%s'\n",
            ExeName);

    return IntGetConsoleAliases(AliasBuffer,
                                AliasBufferLength,
                                ExeName,
                                FALSE);
}


static DWORD
IntGetConsoleAliasesLength(LPCVOID ExeName, BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETALLALIASESLENGTH GetAllAliasesLengthRequest = &ApiMessage.Data.GetAllAliasesLengthRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    USHORT NumChars = (USHORT)(ExeName ? (bUnicode ? wcslen(ExeName) : strlen(ExeName)) : 0);

    if (ExeName == NULL || NumChars == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    GetAllAliasesLengthRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetAllAliasesLengthRequest->ExeLength     = NumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    GetAllAliasesLengthRequest->Unicode  =
    GetAllAliasesLengthRequest->Unicode2 = bUnicode;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, GetAllAliasesLengthRequest->ExeLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)ExeName,
                            GetAllAliasesLengthRequest->ExeLength,
                            (PVOID)&GetAllAliasesLengthRequest->ExeName);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasesLength),
                        sizeof(*GetAllAliasesLengthRequest));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    return GetAllAliasesLengthRequest->Length;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasesLengthW(
    _In_ LPCWSTR ExeName)
{
    return IntGetConsoleAliasesLength(ExeName, TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasesLengthA(
    _In_ LPCSTR ExeName)
{
    return IntGetConsoleAliasesLength(ExeName, FALSE);
}


static DWORD
IntGetConsoleAliasExes(PVOID lpExeNameBuffer,
                       DWORD ExeNameBufferLength,
                       BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETALIASESEXES GetAliasesExesRequest = &ApiMessage.Data.GetAliasesExesRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    GetAliasesExesRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetAliasesExesRequest->Length        = ExeNameBufferLength;
    GetAliasesExesRequest->Unicode       = bUnicode;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, ExeNameBufferLength);
    if (!CaptureBuffer)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              ExeNameBufferLength,
                              (PVOID*)&GetAliasesExesRequest->ExeNames);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasExes),
                        sizeof(*GetAliasesExesRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    RtlCopyMemory(lpExeNameBuffer,
                  GetAliasesExesRequest->ExeNames,
                  GetAliasesExesRequest->Length);

    CsrFreeCaptureBuffer(CaptureBuffer);

    return GetAliasesExesRequest->Length;
}

/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasExesW(LPWSTR lpExeNameBuffer,
                     DWORD ExeNameBufferLength)
{
    DPRINT("GetConsoleAliasExesW called\n");
    return IntGetConsoleAliasExes(lpExeNameBuffer, ExeNameBufferLength, TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasExesA(LPSTR lpExeNameBuffer,
                     DWORD ExeNameBufferLength)
{
    DPRINT("GetConsoleAliasExesA called\n");
    return IntGetConsoleAliasExes(lpExeNameBuffer, ExeNameBufferLength, FALSE);
}


static DWORD
IntGetConsoleAliasExesLength(BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETALIASESEXESLENGTH GetAliasesExesLengthRequest = &ApiMessage.Data.GetAliasesExesLengthRequest;

    GetAliasesExesLengthRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetAliasesExesLengthRequest->Unicode = bUnicode;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetAliasExesLength),
                        sizeof(*GetAliasesExesLengthRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    return GetAliasesExesLengthRequest->Length;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasExesLengthW(VOID)
{
    DPRINT("GetConsoleAliasExesLengthW called\n");
    return IntGetConsoleAliasExesLength(TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleAliasExesLengthA(VOID)
{
    DPRINT("GetConsoleAliasExesLengthA called\n");
    return IntGetConsoleAliasExesLength(FALSE);
}

/* EOF */
