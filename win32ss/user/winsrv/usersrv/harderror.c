/*
 * PROJECT:     ReactOS User API Server DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Hard errors support.
 * COPYRIGHT:   Copyright 2007-2018 Dmitry Philippov (shedon@mail.ru)
 *              Copyright 2010-2018 Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Copyright 2012-2018 Hermes Belusca-Maito
 *              Copyright 2018 Giannis Adamopoulos
 */

/* INCLUDES *******************************************************************/

#include "usersrv.h"

#define NTOS_MODE_USER
#include <ndk/mmfuncs.h>

#include <undocelfapi.h>
#include <ntstrsafe.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/* Cache for the localized hard-error message box strings */
LANGID g_CurrentUserLangId = 0;
UNICODE_STRING g_SuccessU = {0, 0, NULL};
UNICODE_STRING g_InformationU = {0, 0, NULL};
UNICODE_STRING g_WarningU = {0, 0, NULL};
UNICODE_STRING g_ErrorU = {0, 0, NULL};
UNICODE_STRING g_SystemProcessU = {0, 0, NULL};
UNICODE_STRING g_OKTerminateU = {0, 0, NULL};
UNICODE_STRING g_CancelDebugU = {0, 0, NULL};

VOID
RtlLoadUnicodeString(
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID,
    OUT PUNICODE_STRING pUnicodeString,
    IN PCWSTR pDefaultString)
{
    UINT Length;

    /* Try to load the string from the resource */
    Length = LoadStringW(hInstance, uID, (LPWSTR)&pUnicodeString->Buffer, 0);
    if (Length == 0)
    {
        /* If the resource string was not found, use the fallback default one */
        RtlInitUnicodeString(pUnicodeString, pDefaultString);
    }
    else
    {
        /* Set the string length (not NULL-terminated!) */
        pUnicodeString->MaximumLength = (USHORT)(Length * sizeof(WCHAR));
        pUnicodeString->Length = pUnicodeString->MaximumLength;
    }
}


/*
 * NOTE: _scwprintf() is NOT exported by ntdll.dll,
 * only _vscwprintf() is, so we need to implement it here.
 * Code comes from sdk/lib/crt/printf/_scwprintf.c .
 */
int
__cdecl
_scwprintf(
    const wchar_t *format,
    ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = _vscwprintf(format, args);
    va_end(args);

    return len;
}


/* FIXME */
int
WINAPI
MessageBoxTimeoutW(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType,
    WORD wLanguageId,
    DWORD dwTime);


static
VOID
UserpCaptureStringParameters(
    OUT PULONG_PTR Parameters,
    OUT PULONG SizeOfAllUnicodeStrings,
    IN PHARDERROR_MSG Message,
    IN HANDLE hProcess OPTIONAL)
{
    NTSTATUS Status;
    ULONG nParam, Size = 0;
    UNICODE_STRING TempStringU, ParamStringU;
    ANSI_STRING TempStringA;

    if (SizeOfAllUnicodeStrings)
        *SizeOfAllUnicodeStrings = 0;

    /* Read all strings from client space */
    for (nParam = 0; nParam < Message->NumberOfParameters; ++nParam)
    {
        Parameters[nParam] = 0;

        /* Check if the current parameter is a unicode string */
        if (Message->UnicodeStringParameterMask & (1 << nParam))
        {
            /* Skip this string if we do not have a client process */
            if (!hProcess)
                continue;

            /* Read the UNICODE_STRING from the process memory */
            Status = NtReadVirtualMemory(hProcess,
                                         (PVOID)Message->Parameters[nParam],
                                         &ParamStringU,
                                         sizeof(ParamStringU),
                                         NULL);
            if (!NT_SUCCESS(Status))
            {
                /* We failed, skip this string */
                DPRINT1("NtReadVirtualMemory(Message->Parameters) failed, Status 0x%lx, skipping.\n", Status);
                continue;
            }

            /* Allocate a buffer for the string and reserve a NULL terminator */
            TempStringU.MaximumLength = ParamStringU.Length + sizeof(UNICODE_NULL);
            TempStringU.Length = ParamStringU.Length;
            TempStringU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 TempStringU.MaximumLength);
            if (!TempStringU.Buffer)
            {
                /* We failed, skip this string */
                DPRINT1("Cannot allocate memory with size %u, skipping.\n", TempStringU.MaximumLength);
                continue;
            }

            /* Read the string buffer from the process memory */
            Status = NtReadVirtualMemory(hProcess,
                                         ParamStringU.Buffer,
                                         TempStringU.Buffer,
                                         ParamStringU.Length,
                                         NULL);
            if (!NT_SUCCESS(Status))
            {
                /* We failed, skip this string */
                DPRINT1("NtReadVirtualMemory(ParamStringU) failed, Status 0x%lx, skipping.\n", Status);
                RtlFreeHeap(RtlGetProcessHeap(), 0, TempStringU.Buffer);
                continue;
            }
            /* NULL-terminate the string */
            TempStringU.Buffer[TempStringU.Length / sizeof(WCHAR)] = UNICODE_NULL;

            DPRINT("ParamString = \'%wZ\'\n", &TempStringU);

            if (Message->Status == STATUS_SERVICE_NOTIFICATION)
            {
                /* Just keep the allocated NULL-terminated UNICODE string */
                Parameters[nParam] = (ULONG_PTR)TempStringU.Buffer;
                Size += TempStringU.Length;
            }
            else
            {
                /* Allocate a buffer for conversion to ANSI string */
                TempStringA.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(&TempStringU);
                TempStringA.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     TempStringA.MaximumLength);
                if (!TempStringA.Buffer)
                {
                    /* We failed, skip this string */
                    DPRINT1("Cannot allocate memory with size %u, skipping.\n", TempStringA.MaximumLength);
                    RtlFreeHeap(RtlGetProcessHeap(), 0, TempStringU.Buffer);
                    continue;
                }

                /* Convert string to ANSI and free temporary buffer */
                Status = RtlUnicodeStringToAnsiString(&TempStringA, &TempStringU, FALSE);
                RtlFreeHeap(RtlGetProcessHeap(), 0, TempStringU.Buffer);
                if (!NT_SUCCESS(Status))
                {
                    /* We failed, skip this string */
                    DPRINT1("RtlUnicodeStringToAnsiString() failed, Status 0x%lx, skipping.\n", Status);
                    RtlFreeHeap(RtlGetProcessHeap(), 0, TempStringA.Buffer);
                    continue;
                }

                /* Note: RtlUnicodeStringToAnsiString() returns a NULL-terminated string */
                Parameters[nParam] = (ULONG_PTR)TempStringA.Buffer;
                Size += TempStringU.Length;
            }
        }
        else
        {
            /* It's not a unicode string, just copy the parameter */
            Parameters[nParam] = Message->Parameters[nParam];
        }
    }

    if (SizeOfAllUnicodeStrings)
        *SizeOfAllUnicodeStrings = Size;
}

static
VOID
UserpFreeStringParameters(
    IN OUT PULONG_PTR Parameters,
    IN PHARDERROR_MSG Message)
{
    ULONG nParam;

    /* Loop all parameters */
    for (nParam = 0; nParam < Message->NumberOfParameters; ++nParam)
    {
        /* Check if the current parameter is a string */
        if ((Message->UnicodeStringParameterMask & (1 << nParam)) && (Parameters[nParam] != 0))
        {
            /* Free the string buffer */
            RtlFreeHeap(RtlGetProcessHeap(), 0, (PVOID)Parameters[nParam]);
        }
    }
}

static
NTSTATUS
UserpGetClientFileName(
    OUT PUNICODE_STRING ClientFileNameU,
    IN HANDLE hProcess)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    PPEB_LDR_DATA Ldr;
    PROCESS_BASIC_INFORMATION ClientBasicInfo;
    LDR_DATA_TABLE_ENTRY ModuleData;
    PVOID ClientDllBase;
    NTSTATUS Status;
    PPEB Peb;

    /* Initialize string */
    RtlInitEmptyUnicodeString(ClientFileNameU, NULL, 0);

    /* Query process information */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ClientBasicInfo,
                                       sizeof(ClientBasicInfo),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Locate the process loader data table and retrieve its name from it */

    Peb = ClientBasicInfo.PebBaseAddress;
    if (!Peb) return STATUS_UNSUCCESSFUL;

    Status = NtReadVirtualMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL);
    if (!NT_SUCCESS(Status)) return Status;

    ModuleListHead = &Ldr->InLoadOrderModuleList;
    Status = NtReadVirtualMemory(hProcess,
                                 &ModuleListHead->Flink,
                                 &Entry,
                                 sizeof(Entry),
                                 NULL);
    if (!NT_SUCCESS(Status)) return Status;

    if (Entry == ModuleListHead) return STATUS_UNSUCCESSFUL;

    Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    Status = NtReadVirtualMemory(hProcess,
                                 Module,
                                 &ModuleData,
                                 sizeof(ModuleData),
                                 NULL);
    if (!NT_SUCCESS(Status)) return Status;

    Status = NtReadVirtualMemory(hProcess,
                                 &Peb->ImageBaseAddress,
                                 &ClientDllBase,
                                 sizeof(ClientDllBase),
                                 NULL);
    if (!NT_SUCCESS(Status)) return Status;

    if (ClientDllBase != ModuleData.DllBase) return STATUS_UNSUCCESSFUL;

    ClientFileNameU->MaximumLength = ModuleData.BaseDllName.MaximumLength;
    ClientFileNameU->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              ClientFileNameU->MaximumLength);
    if (!ClientFileNameU->Buffer)
    {
        RtlInitEmptyUnicodeString(ClientFileNameU, NULL, 0);
        return STATUS_NO_MEMORY;
    }

    Status = NtReadVirtualMemory(hProcess,
                                 ModuleData.BaseDllName.Buffer,
                                 ClientFileNameU->Buffer,
                                 ClientFileNameU->MaximumLength,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ClientFileNameU->Buffer);
        RtlInitEmptyUnicodeString(ClientFileNameU, NULL, 0);
        return Status;
    }

    ClientFileNameU->Length = (USHORT)(wcslen(ClientFileNameU->Buffer) * sizeof(WCHAR));
    DPRINT("ClientFileNameU = \'%wZ\'\n", &ClientFileNameU);

    return STATUS_SUCCESS;
}

static
VOID
UserpDuplicateParamStringToUnicodeString(
    IN OUT PUNICODE_STRING UnicodeString,
    IN PCWSTR ParamString)
{
    UNICODE_STRING FormatU, TempStringU;

    /* Calculate buffer length for the text message */
    RtlInitUnicodeString(&FormatU, (PWSTR)ParamString);
    if (UnicodeString->MaximumLength < FormatU.MaximumLength)
    {
        /* Duplicate the text message in a larger buffer */
        if (NT_SUCCESS(RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                                 &FormatU, &TempStringU)))
        {
            *UnicodeString = TempStringU;
        }
        else
        {
            /* We could not allocate a larger buffer; continue using the smaller original buffer */
            DPRINT1("Cannot allocate memory for UnicodeString, use original buffer.\n");

            /* Copy the truncated string, NULL-terminate it */
            FormatU.MaximumLength = UnicodeString->MaximumLength;
            FormatU.Length = FormatU.MaximumLength - sizeof(UNICODE_NULL);
            RtlCopyUnicodeString(UnicodeString, &FormatU);
        }
    }
    else
    {
        /* Copy the string, NULL-terminate it */
        RtlCopyUnicodeString(UnicodeString, &FormatU);
    }
}

static
VOID
UserpFormatMessages(
    IN OUT PUNICODE_STRING TextStringU,
    IN OUT PUNICODE_STRING CaptionStringU,
    OUT PUINT pdwType,
    OUT PULONG pdwTimeout,
    IN PHARDERROR_MSG Message)
{
    /* Special hardcoded messages */
    static const PCWSTR pszUnknownHardError =
        L"Unknown Hard Error 0x%08lx\n"
        L"Parameters: 0x%p 0x%p 0x%p 0x%p";
    static const PCWSTR pszExceptionHardError =
        L"Exception processing message 0x%08lx\n"
        L"Parameters: 0x%p 0x%p 0x%p 0x%p";

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hProcess;
    ULONG Severity = (ULONG)(Message->Status) >> 30;
    ULONG SizeOfStrings;
    ULONG_PTR Parameters[MAXIMUM_HARDERROR_PARAMETERS] = {0};
    ULONG_PTR CopyParameters[MAXIMUM_HARDERROR_PARAMETERS];
    UNICODE_STRING WindowTitleU, FileNameU, TempStringU, FormatU, Format2U;
    ANSI_STRING FormatA, Format2A;
    HWND hwndOwner;
    PMESSAGE_RESOURCE_ENTRY MessageResource;
    PWSTR FormatString, pszBuffer;
    size_t cszBuffer;

    /* Open client process */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&hProcess,
                           PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &Message->h.ClientId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenProcess failed with status 0x%08lx, possibly SYSTEM process.\n", Status);
        hProcess = NULL;
    }

    /* Capture all string parameters from the process memory */
    UserpCaptureStringParameters(Parameters, &SizeOfStrings, Message, hProcess);

    /* Initialize the output strings */
    TextStringU->Length = 0;
    TextStringU->Buffer[0] = UNICODE_NULL;

    CaptionStringU->Length = 0;
    CaptionStringU->Buffer[0] = UNICODE_NULL;

    /*
     * Check whether it is a service notification, in which case
     * we format the parameters and take the short route.
     */
    if (Message->Status == STATUS_SERVICE_NOTIFICATION)
    {
        /* Close the process handle */
        if (hProcess) NtClose(hProcess);

        /*
         * Retrieve the message box flags. Note that we filter out
         * MB_SERVICE_NOTIFICATION to not enter an infinite recursive
         * loop when we will call MessageBox() later on.
         */
        *pdwType = (UINT)Parameters[2] & ~MB_SERVICE_NOTIFICATION;

        /*
         * Duplicate the UNICODE text message and caption.
         * If no strings or invalid ones have been provided, keep
         * the original buffers and reset the string lengths to zero.
         */
        if (Message->UnicodeStringParameterMask & 0x1)
            UserpDuplicateParamStringToUnicodeString(TextStringU, (PCWSTR)Parameters[0]);
        if (Message->UnicodeStringParameterMask & 0x2)
            UserpDuplicateParamStringToUnicodeString(CaptionStringU, (PCWSTR)Parameters[1]);

        /* Set the timeout */
        if (Message->NumberOfParameters >= 4)
            *pdwTimeout = (ULONG)Parameters[3];
        else
            *pdwTimeout = INFINITE;

        goto Quit;
    }

    /* Set the message box type */
    *pdwType = 0;
    switch (Message->ValidResponseOptions)
    {
        case OptionAbortRetryIgnore:
            *pdwType = MB_ABORTRETRYIGNORE;
            break;
        case OptionOk:
            *pdwType = MB_OK;
            break;
        case OptionOkCancel:
            *pdwType = MB_OKCANCEL;
            break;
        case OptionRetryCancel:
            *pdwType = MB_RETRYCANCEL;
            break;
        case OptionYesNo:
            *pdwType = MB_YESNO;
            break;
        case OptionYesNoCancel:
            *pdwType = MB_YESNOCANCEL;
            break;
        case OptionShutdownSystem:
            *pdwType = MB_OK;
            break;
        case OptionOkNoWait:
            *pdwType = MB_OK;
            break;
        case OptionCancelTryContinue:
            *pdwType = MB_CANCELTRYCONTINUE;
            break;
    }

    /* Set the severity icon */
    // STATUS_SEVERITY_SUCCESS
         if (Severity == STATUS_SEVERITY_INFORMATIONAL) *pdwType |= MB_ICONINFORMATION;
    else if (Severity == STATUS_SEVERITY_WARNING)       *pdwType |= MB_ICONWARNING;
    else if (Severity == STATUS_SEVERITY_ERROR)         *pdwType |= MB_ICONERROR;

    *pdwType |= MB_SYSTEMMODAL | MB_SETFOREGROUND;

    /* Set the timeout */
    *pdwTimeout = INFINITE;

    /* Copy the Parameters array locally */
    RtlCopyMemory(&CopyParameters, Parameters, sizeof(CopyParameters));

    /* Get the file name of the client process */
    Status = STATUS_SUCCESS;
    if (hProcess)
        Status = UserpGetClientFileName(&FileNameU, hProcess);

    /* Close the process handle but keep its original value to know where stuff came from */
    if (hProcess) NtClose(hProcess);

    /*
     * Fall back to SYSTEM process if the client process handle
     * was NULL or we failed retrieving a file name.
     */
    if (!hProcess || !NT_SUCCESS(Status) || !FileNameU.Buffer)
    {
        hProcess  = NULL;
        FileNameU = g_SystemProcessU;
    }

    /* Retrieve the description of the error code */
    FormatA.Buffer = NULL;
    Status = RtlFindMessage(GetModuleHandleW(L"ntdll"),
                            (ULONG_PTR)RT_MESSAGETABLE,
                            LANG_NEUTRAL,
                            Message->Status,
                            &MessageResource);
    if (NT_SUCCESS(Status))
    {
        if (MessageResource->Flags)
        {
            RtlInitUnicodeString(&FormatU, (PWSTR)MessageResource->Text);
            FormatA.Buffer = NULL;
        }
        else
        {
            RtlInitAnsiString(&FormatA, (PSTR)MessageResource->Text);
            /* Status = */ RtlAnsiStringToUnicodeString(&FormatU, &FormatA, TRUE);
        }
        ASSERT(FormatU.Buffer);
    }
    else
    {
        /*
         * Fall back to unknown hard error format string.
         * NOTE: The value used here is ReactOS-specific: it allows specifying
         * the exact hard error status value and the parameters, contrary to
         * the one on Windows that only says: "Unknown Hard Error".
         */
        RtlInitEmptyUnicodeString(&FormatU, NULL, 0);
        FormatA.Buffer = NULL;
    }

    FormatString = FormatU.Buffer;

    /* Check whether a caption is specified in the format string */
    if (FormatString && FormatString[0] == L'{')
    {
        /* Set caption start */
        TempStringU.Buffer = ++FormatString;

        /* Get the caption size and find where the format string really starts */
        for (TempStringU.Length = 0;
             *FormatString != UNICODE_NULL && *FormatString != L'}';
             ++TempStringU.Length)
        {
            ++FormatString;
        }

        /* Skip '}', '\r', '\n' */
        FormatString += 3;

        TempStringU.Length *= sizeof(WCHAR);
        TempStringU.MaximumLength = TempStringU.Length;
    }
    else
    {
        if (Severity == STATUS_SEVERITY_SUCCESS)
            TempStringU = g_SuccessU;
        else if (Severity == STATUS_SEVERITY_INFORMATIONAL)
            TempStringU = g_InformationU;
        else if (Severity == STATUS_SEVERITY_WARNING)
            TempStringU = g_WarningU;
        else if (Severity == STATUS_SEVERITY_ERROR)
            TempStringU = g_ErrorU;
        else
            ASSERT(FALSE); // Unexpected, since Severity is only <= 3.
    }

    /* Retrieve the window title of the client, if it has one */
    RtlInitEmptyUnicodeString(&WindowTitleU, L"", 0);
    hwndOwner = NULL;
    EnumThreadWindows(HandleToUlong(Message->h.ClientId.UniqueThread),
                      FindTopLevelWnd, (LPARAM)&hwndOwner);
    if (hwndOwner)
    {
        cszBuffer = GetWindowTextLengthW(hwndOwner);
        if (cszBuffer != 0)
        {
            cszBuffer += 3; // 2 characters for ": " and a NULL terminator.
            cszBuffer *= sizeof(WCHAR);
            pszBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        cszBuffer);
            if (pszBuffer)
            {
                RtlInitEmptyUnicodeString(&WindowTitleU, pszBuffer, (USHORT)cszBuffer);
                cszBuffer = GetWindowTextW(hwndOwner,
                                           WindowTitleU.Buffer,
                                           WindowTitleU.MaximumLength / sizeof(WCHAR));
                WindowTitleU.Length = (USHORT)(cszBuffer * sizeof(WCHAR));
                RtlAppendUnicodeToString(&WindowTitleU, L": ");
            }
        }
    }

    /* Calculate buffer length for the caption */
    cszBuffer = WindowTitleU.Length + FileNameU.Length + TempStringU.Length +
                3 * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    if (CaptionStringU->MaximumLength < cszBuffer)
    {
        /* Allocate a larger buffer for the caption */
        pszBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    cszBuffer);
        if (!pszBuffer)
        {
            /* We could not allocate a larger buffer; continue using the smaller original buffer */
            DPRINT1("Cannot allocate memory for CaptionStringU, use original buffer.\n");
        }
        else
        {
            RtlInitEmptyUnicodeString(CaptionStringU, pszBuffer, (USHORT)cszBuffer);
        }
    }
    CaptionStringU->Length = 0;
    CaptionStringU->Buffer[0] = UNICODE_NULL;

    /* Build the caption */
    RtlStringCbPrintfW(CaptionStringU->Buffer,
                       CaptionStringU->MaximumLength,
                       L"%wZ%wZ - %wZ",
                       &WindowTitleU, &FileNameU, &TempStringU);
    CaptionStringU->Length = (USHORT)(wcslen(CaptionStringU->Buffer) * sizeof(WCHAR));

    /* Free the strings if needed */
    if (WindowTitleU.Buffer && (WindowTitleU.MaximumLength != 0))
        RtlFreeUnicodeString(&WindowTitleU);
    if (hProcess)
        RtlFreeUnicodeString(&FileNameU);

    Format2A.Buffer = NULL;

    /* If we have an unknown hard error, skip the special cases handling */
    if (!FormatString)
        goto BuildMessage;

    /* Check if this is an exception message */
    if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
    {
        ULONG ExceptionCode = CopyParameters[0];

        /* Retrieve the description of the exception code */
        Status = RtlFindMessage(GetModuleHandleW(L"ntdll"),
                                (ULONG_PTR)RT_MESSAGETABLE,
                                LANG_NEUTRAL,
                                ExceptionCode,
                                &MessageResource);
        if (NT_SUCCESS(Status))
        {
            if (MessageResource->Flags)
            {
                RtlInitUnicodeString(&Format2U, (PWSTR)MessageResource->Text);
                Format2A.Buffer = NULL;
            }
            else
            {
                RtlInitAnsiString(&Format2A, (PSTR)MessageResource->Text);
                /* Status = */ RtlAnsiStringToUnicodeString(&Format2U, &Format2A, TRUE);
            }
            ASSERT(Format2U.Buffer);

            /* Handle special cases */
            if (ExceptionCode == STATUS_ACCESS_VIOLATION)
            {
                /* Use a new FormatString */
                FormatString = Format2U.Buffer;
                CopyParameters[0] = CopyParameters[1];
                CopyParameters[1] = CopyParameters[3];
                if (CopyParameters[2])
                    CopyParameters[2] = (ULONG_PTR)L"written";
                else
                    CopyParameters[2] = (ULONG_PTR)L"read";
            }
            else if (ExceptionCode == STATUS_IN_PAGE_ERROR)
            {
                /* Use a new FormatString */
                FormatString = Format2U.Buffer;
                CopyParameters[0] = CopyParameters[1];
                CopyParameters[1] = CopyParameters[3];
            }
            else
            {
                /* Keep the existing FormatString */
                CopyParameters[2] = CopyParameters[1];
                CopyParameters[1] = CopyParameters[0];

                pszBuffer = Format2U.Buffer;
                if (!_wcsnicmp(pszBuffer, L"{EXCEPTION}", 11))
                {
                    /*
                     * This is a named exception. Skip the mark and
                     * retrieve the exception name that follows it.
                     */
                    pszBuffer += 11;

                    /* Skip '\r', '\n' */
                    pszBuffer += 2;

                    CopyParameters[0] = (ULONG_PTR)pszBuffer;
                }
                else
                {
                    /* Fall back to hardcoded value */
                    CopyParameters[0] = (ULONG_PTR)L"unknown software exception";
                }
            }
        }
        else
        {
            /* Fall back to hardcoded value, and keep the existing FormatString */
            CopyParameters[2] = CopyParameters[1];
            CopyParameters[1] = CopyParameters[0];
            CopyParameters[0] = (ULONG_PTR)L"unknown software exception";
        }
    }

BuildMessage:
    /*
     * Calculate buffer length for the text message. If FormatString
     * is NULL this means we have an unknown hard error whose format
     * string is in FormatU.
     */
    cszBuffer = 0;
    /* Wrap in SEH to protect from invalid string parameters */
    _SEH2_TRY
    {
        if (!FormatString)
        {
            /* Fall back to unknown hard error format string, and use the original parameters */
            cszBuffer = _scwprintf(pszUnknownHardError,
                                   Message->Status,
                                   Parameters[0], Parameters[1],
                                   Parameters[2], Parameters[3]);
            cszBuffer *= sizeof(WCHAR);
        }
        else
        {
            cszBuffer = _scwprintf(FormatString,
                                   CopyParameters[0], CopyParameters[1],
                                   CopyParameters[2], CopyParameters[3]);
            cszBuffer *= sizeof(WCHAR);

            /* Add a description for the dialog buttons */
            if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
            {
                if (Message->ValidResponseOptions == OptionOk ||
                    Message->ValidResponseOptions == OptionOkCancel)
                {
                    /* Reserve space for one newline and the OK-terminate-program string */
                    cszBuffer += sizeof(WCHAR) + g_OKTerminateU.Length;
                }
                if (Message->ValidResponseOptions == OptionOkCancel)
                {
                    /* Reserve space for one newline and the CANCEL-debug-program string */
                    cszBuffer += sizeof(WCHAR) + g_CancelDebugU.Length;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* An exception occurred, use a default string with the original parameters */
        cszBuffer = _scwprintf(pszExceptionHardError,
                               Message->Status,
                               Parameters[0], Parameters[1],
                               Parameters[2], Parameters[3]);
        cszBuffer *= sizeof(WCHAR);
    }
    _SEH2_END;

    cszBuffer += SizeOfStrings + sizeof(UNICODE_NULL);

    if (TextStringU->MaximumLength < cszBuffer)
    {
        /* Allocate a larger buffer for the text message */
        pszBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    cszBuffer);
        if (!pszBuffer)
        {
            /* We could not allocate a larger buffer; continue using the smaller original buffer */
            DPRINT1("Cannot allocate memory for TextStringU, use original buffer.\n");
        }
        else
        {
            RtlInitEmptyUnicodeString(TextStringU, pszBuffer, (USHORT)cszBuffer);
        }
    }
    TextStringU->Length = 0;
    TextStringU->Buffer[0] = UNICODE_NULL;

    /* Wrap in SEH to protect from invalid string parameters */
    _SEH2_TRY
    {
        /* Print the string into the buffer */
        pszBuffer = TextStringU->Buffer;
        cszBuffer = TextStringU->MaximumLength;

        if (!FormatString)
        {
            /* Fall back to unknown hard error format string, and use the original parameters */
            RtlStringCbPrintfW(pszBuffer, cszBuffer,
                               pszUnknownHardError,
                               Message->Status,
                               Parameters[0], Parameters[1],
                               Parameters[2], Parameters[3]);
        }
        else
        {
            RtlStringCbPrintfExW(pszBuffer, cszBuffer,
                                 &pszBuffer, &cszBuffer,
                                 0,
                                 FormatString,
                                 CopyParameters[0], CopyParameters[1],
                                 CopyParameters[2], CopyParameters[3]);

            /* Add a description for the dialog buttons */
            if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
            {
                if (Message->ValidResponseOptions == OptionOk ||
                    Message->ValidResponseOptions == OptionOkCancel)
                {
                    RtlStringCbPrintfExW(pszBuffer, cszBuffer,
                                         &pszBuffer, &cszBuffer,
                                         0,
                                         L"\n%wZ",
                                         &g_OKTerminateU);
                }
                if (Message->ValidResponseOptions == OptionOkCancel)
                {
                    RtlStringCbPrintfExW(pszBuffer, cszBuffer,
                                         &pszBuffer, &cszBuffer,
                                         0,
                                         L"\n%wZ",
                                         &g_CancelDebugU);
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* An exception occurred, use a default string with the original parameters */
        DPRINT1("Exception 0x%08lx occurred while building hard-error message, fall back to default message.\n",
                _SEH2_GetExceptionCode());

        RtlStringCbPrintfW(TextStringU->Buffer,
                           TextStringU->MaximumLength,
                           pszExceptionHardError,
                           Message->Status,
                           Parameters[0], Parameters[1],
                           Parameters[2], Parameters[3]);
    }
    _SEH2_END;

    TextStringU->Length = (USHORT)(wcslen(TextStringU->Buffer) * sizeof(WCHAR));

    /* Free the converted UNICODE strings */
    if (Format2A.Buffer) RtlFreeUnicodeString(&Format2U);
    if (FormatA.Buffer) RtlFreeUnicodeString(&FormatU);

Quit:
    /* Free the captured parameters */
    UserpFreeStringParameters(Parameters, Message);
}

static ULONG
GetRegInt(
    IN PCWSTR KeyName,
    IN PCWSTR ValueName,
    IN ULONG  DefaultValue)
{
    NTSTATUS Status;
    ULONG Value = DefaultValue;
    UNICODE_STRING String;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    ULONG ResultLength;
    UCHAR ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;

    RtlInitUnicodeString(&String, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &String,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the registry key */
    Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Query the value */
        RtlInitUnicodeString(&String, ValueName);
        Status = NtQueryValueKey(KeyHandle,
                                 &String,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(ValueBuffer),
                                 &ResultLength);

        /* Close the registry key */
        NtClose(KeyHandle);

        if (NT_SUCCESS(Status) && (ValueInfo->Type == REG_DWORD))
        {
            /* Directly retrieve the data */
            Value = *(PULONG)ValueInfo->Data;
        }
    }

    return Value;
}

static BOOL
UserpShowInformationBalloon(
    IN PUNICODE_STRING TextStringU,
    IN PUNICODE_STRING CaptionStringU,
    IN UINT Type,
    IN PHARDERROR_MSG Message)
{
    ULONG ShellErrorMode;
    HWND hWndTaskman;
    COPYDATASTRUCT CopyData;
    PBALLOON_HARD_ERROR_DATA pdata;
    DWORD dwSize, cbTextLen, cbTitleLen;
    PWCHAR pText, pCaption;
    DWORD ret;
    DWORD_PTR dwResult;

    /* Query the shell error mode value */
    ShellErrorMode = GetRegInt(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Windows",
                               L"ShellErrorMode", 0);

    /* Make the shell display the hard error message only if necessary */
    if (ShellErrorMode != 1)
        return FALSE;

    /* Retrieve the shell task window */
    hWndTaskman = GetTaskmanWindow();
    if (!hWndTaskman)
    {
        DPRINT1("Failed to find shell task window (last error %lu)\n", GetLastError());
        return FALSE;
    }

    cbTextLen  = TextStringU->Length + sizeof(UNICODE_NULL);
    cbTitleLen = CaptionStringU->Length + sizeof(UNICODE_NULL);

    dwSize = sizeof(BALLOON_HARD_ERROR_DATA);
    dwSize += cbTextLen + cbTitleLen;

    /* Build the data buffer */
    pdata = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!pdata)
    {
        DPRINT1("Failed to allocate balloon data\n");
        return FALSE;
    }

    pdata->cbHeaderSize = sizeof(BALLOON_HARD_ERROR_DATA);
    pdata->Status = Message->Status;
    pdata->dwType = Type;

    pdata->TitleOffset   = pdata->cbHeaderSize;
    pdata->MessageOffset = pdata->TitleOffset + cbTitleLen;
    pCaption = (PWCHAR)((ULONG_PTR)pdata + pdata->TitleOffset);
    pText = (PWCHAR)((ULONG_PTR)pdata + pdata->MessageOffset);
    RtlStringCbCopyNW(pCaption, cbTitleLen, CaptionStringU->Buffer, CaptionStringU->Length);
    RtlStringCbCopyNW(pText, cbTextLen, TextStringU->Buffer, TextStringU->Length);

    /* Send the message */

    /* Retrieve a unique system-wide message to communicate hard error data with the shell */
    CopyData.dwData = RegisterWindowMessageW(L"HardError");
    CopyData.cbData = dwSize;
    CopyData.lpData = pdata;

    dwResult = FALSE;
    ret = SendMessageTimeoutW(hWndTaskman, WM_COPYDATA, 0, (LPARAM)&CopyData,
                              SMTO_NORMAL | SMTO_ABORTIFHUNG, 3000, &dwResult);

    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, pdata);

    return (ret && dwResult) ? TRUE : FALSE;
}

static
HARDERROR_RESPONSE
UserpMessageBox(
    IN PUNICODE_STRING TextStringU,
    IN PUNICODE_STRING CaptionStringU,
    IN UINT  Type,
    IN ULONG Timeout)
{
    ULONG MessageBoxResponse;

    DPRINT("Text = '%S', Caption = '%S', Type = 0x%lx\n",
           TextStringU->Buffer, CaptionStringU->Buffer, Type);

    /* Display a message box */
    MessageBoxResponse = MessageBoxTimeoutW(NULL,
                                            TextStringU->Buffer,
                                            CaptionStringU->Buffer,
                                            Type, 0, Timeout);

    /* Return response value */
    switch (MessageBoxResponse)
    {
        case IDOK:       return ResponseOk;
        case IDCANCEL:   return ResponseCancel;
        case IDYES:      return ResponseYes;
        case IDNO:       return ResponseNo;
        case IDABORT:    return ResponseAbort;
        case IDIGNORE:   return ResponseIgnore;
        case IDRETRY:    return ResponseRetry;
        case IDTRYAGAIN: return ResponseTryAgain;
        case IDCONTINUE: return ResponseContinue;
        default:         return ResponseNotHandled;
    }

    return ResponseNotHandled;
}

static
VOID
UserpLogHardError(
    IN PUNICODE_STRING TextStringU,
    IN PUNICODE_STRING CaptionStringU)
{
    NTSTATUS Status;
    HANDLE hEventLog;
    UNICODE_STRING UNCServerNameU = {0, 0, NULL};
    UNICODE_STRING SourceNameU = RTL_CONSTANT_STRING(L"Application Popup");
    PUNICODE_STRING Strings[] = {CaptionStringU, TextStringU};

    Status = ElfRegisterEventSourceW(&UNCServerNameU, &SourceNameU, &hEventLog);
    if (!NT_SUCCESS(Status) || !hEventLog)
    {
        DPRINT1("ElfRegisterEventSourceW failed with Status 0x%08lx\n", Status);
        return;
    }

    Status = ElfReportEventW(hEventLog,
                             EVENTLOG_INFORMATION_TYPE,
                             0,
                             STATUS_LOG_HARD_ERROR,
                             NULL,      // lpUserSid
                             ARRAYSIZE(Strings),
                             0,         // dwDataSize
                             Strings,
                             NULL,      // lpRawData
                             0,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("ElfReportEventW failed with Status 0x%08lx\n", Status);

    ElfDeregisterEventSource(hEventLog);
}

VOID
NTAPI
UserServerHardError(
    IN PCSR_THREAD ThreadData,
    IN PHARDERROR_MSG Message)
{
    ULONG ErrorMode;
    UINT  dwType = 0;
    ULONG Timeout = INFINITE;
    UNICODE_STRING TextU, CaptionU;
    WCHAR LocalTextBuffer[256];
    WCHAR LocalCaptionBuffer[256];
    NTSTATUS Status;

    ASSERT(ThreadData->Process != NULL);

    /* Default to not handled */
    Message->Response = ResponseNotHandled;

    /* Make sure we don't have too many parameters */
    if (Message->NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
    {
        // NOTE: Windows just fails (STATUS_INVALID_PARAMETER) & returns ResponseNotHandled.
        DPRINT1("Invalid NumberOfParameters = %d\n", Message->NumberOfParameters);
        Message->NumberOfParameters = MAXIMUM_HARDERROR_PARAMETERS;
    }
    if (Message->ValidResponseOptions > OptionCancelTryContinue)
    {
        DPRINT1("Unknown ValidResponseOptions = %d\n", Message->ValidResponseOptions);
        return; // STATUS_INVALID_PARAMETER;
    }
    if (Message->Status == STATUS_SERVICE_NOTIFICATION)
    {
        if (Message->NumberOfParameters < 3)
        {
            DPRINT1("Invalid NumberOfParameters = %d for STATUS_SERVICE_NOTIFICATION\n",
                    Message->NumberOfParameters);
            return; // STATUS_INVALID_PARAMETER;
        }
        // (Message->UnicodeStringParameterMask & 0x3)
    }

    Status = NtUserSetInformationThread(NtCurrentThread(),
                                        UserThreadUseActiveDesktop,
                                        NULL,
                                        0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set thread desktop!\n");
        return;
    }

    /* Re-initialize the hard errors cache */
    UserInitHardErrorsCache();

    /* Format the message caption and text */
    RtlInitEmptyUnicodeString(&TextU, LocalTextBuffer, sizeof(LocalTextBuffer));
    RtlInitEmptyUnicodeString(&CaptionU, LocalCaptionBuffer, sizeof(LocalCaptionBuffer));
    UserpFormatMessages(&TextU, &CaptionU, &dwType, &Timeout, Message);

    /* Log the hard error message */
    UserpLogHardError(&TextU, &CaptionU);

    /* Display a hard error popup depending on the current ErrorMode */

    /* Query the error mode value */
    ErrorMode = GetRegInt(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Windows",
                          L"ErrorMode", 0);

    if (Message->Status != STATUS_SERVICE_NOTIFICATION && ErrorMode != 0)
    {
        /* Returns OK for the hard error */
        Message->Response = ResponseOk;
        goto Quit;
    }

    if (Message->ValidResponseOptions == OptionOkNoWait)
    {
        /* Display the balloon */
        Message->Response = ResponseOk;
        if (UserpShowInformationBalloon(&TextU,
                                        &CaptionU,
                                        dwType,
                                        Message))
        {
            Message->Response = ResponseOk;
            goto Quit;
        }
    }

    /* Display the message box */
    Message->Response = UserpMessageBox(&TextU,
                                        &CaptionU,
                                        dwType,
                                        Timeout);

Quit:
    /* Free the strings if they have been reallocated */
    if (TextU.Buffer != LocalTextBuffer)
        RtlFreeUnicodeString(&TextU);
    if (CaptionU.Buffer != LocalCaptionBuffer)
        RtlFreeUnicodeString(&CaptionU);

    NtUserSetInformationThread(NtCurrentThread(), UserThreadRestoreDesktop, NULL, 0);

    return;
}

VOID
UserInitHardErrorsCache(VOID)
{
    NTSTATUS Status;
    LCID CurrentUserLCID = 0;

    Status = NtQueryDefaultLocale(TRUE, &CurrentUserLCID);
    if (!NT_SUCCESS(Status) || CurrentUserLCID == 0)
    {
        /* Fall back to english locale */
        DPRINT1("NtQueryDefaultLocale failed with Status = 0x%08lx\n", Status);
        // LOCALE_SYSTEM_DEFAULT;
        CurrentUserLCID = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    }
    if (g_CurrentUserLangId == LANGIDFROMLCID(CurrentUserLCID))
    {
        /* The current lang ID and the hard error strings have already been cached */
        return;
    }

    /* Load the strings using the current system locale */
    RtlLoadUnicodeString(UserServerDllInstance, IDS_SEVERITY_SUCCESS,
                         &g_SuccessU, L"Success");
    RtlLoadUnicodeString(UserServerDllInstance, IDS_SEVERITY_INFORMATIONAL,
                         &g_InformationU, L"System Information");
    RtlLoadUnicodeString(UserServerDllInstance, IDS_SEVERITY_WARNING,
                         &g_WarningU, L"System Warning");
    RtlLoadUnicodeString(UserServerDllInstance, IDS_SEVERITY_ERROR,
                         &g_ErrorU, L"System Error");
    // "unknown software exception"
    RtlLoadUnicodeString(UserServerDllInstance, IDS_SYSTEM_PROCESS,
                         &g_SystemProcessU, L"System Process");
    RtlLoadUnicodeString(UserServerDllInstance, IDS_OK_TERMINATE_PROGRAM,
                         &g_OKTerminateU, L"Click on OK to terminate the program.");
    RtlLoadUnicodeString(UserServerDllInstance, IDS_CANCEL_DEBUG_PROGRAM,
                         &g_CancelDebugU, L"Click on CANCEL to debug the program.");

    /* Remember that we cached the hard error strings */
    g_CurrentUserLangId = LANGIDFROMLCID(CurrentUserLCID);
}

/* EOF */
