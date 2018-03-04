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

    ClientFileNameU->Length = wcslen(ClientFileNameU->Buffer) * sizeof(WCHAR);
    DPRINT("ClientFileNameU = \'%wZ\'\n", &ClientFileNameU);

    return STATUS_SUCCESS;
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

            /* Allocate a buffer for the string */
            TempStringU.MaximumLength = ParamStringU.Length;
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

            DPRINT("ParamString = \'%wZ\'\n", &TempStringU);

            /* Allocate a buffer for converted to ANSI string */
            TempStringA.MaximumLength = RtlUnicodeStringToAnsiSize(&TempStringU);
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

            /* Note: RtlUnicodeStringToAnsiString returns NULL terminated string */
            Parameters[nParam] = (ULONG_PTR)TempStringA.Buffer;
            Size += TempStringU.Length;
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
NTSTATUS
UserpFormatMessages(
    OUT PUNICODE_STRING TextStringU,
    OUT PUNICODE_STRING CaptionStringU,
    IN  PULONG_PTR Parameters,
    IN  ULONG SizeOfStrings,
    IN  PHARDERROR_MSG Message,
    IN  HANDLE hProcess OPTIONAL)
{
    NTSTATUS Status;
    UNICODE_STRING FileNameU, TempStringU, WindowTitleU, FormatU, Format2U;
    ANSI_STRING FormatA, Format2A;
    HWND hwndOwner;
    PMESSAGE_RESOURCE_ENTRY MessageResource;
    ULONG_PTR CapturedParameters[MAXIMUM_HARDERROR_PARAMETERS];
    PWSTR FormatString, pszBuffer;
    size_t cchBuffer;
    ULONG ExceptionCode, Severity;
    ULONG Size;

    /* Copy the Parameters array locally */
    RtlCopyMemory(&CapturedParameters, Parameters, sizeof(CapturedParameters));

    /* Get the file name of the client process */
    Status = STATUS_SUCCESS;
    if (hProcess)
        Status = UserpGetClientFileName(&FileNameU, hProcess);

    /*
     * Fall back to SYSTEM process if the client process handle
     * was NULL or we failed retrieving a file name.
     */
    if (!hProcess || !NT_SUCCESS(Status) || !FileNameU.Buffer)
    {
        hProcess  = NULL;
        FileNameU = g_SystemProcessU;
    }

    Severity = (ULONG)(Message->Status) >> 30;

    /* Get text string of the error code */
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
            RtlInitAnsiString(&FormatA, (PCHAR)MessageResource->Text);
            /* Status = */ RtlAnsiStringToUnicodeString(&FormatU, &FormatA, TRUE);
        }
    }
    else
    {
        /* Fall back to hardcoded value */
        RtlInitUnicodeString(&FormatU, L"Unknown Hard Error");
        FormatA.Buffer = NULL;
    }

    FormatString = FormatU.Buffer;

    /* Check whether a caption exists */
    if (FormatString[0] == L'{')
    {
        /* Set caption start */
        TempStringU.Buffer = ++FormatString;

        /* Get size of the caption */
        for (Size = 0; *FormatString != UNICODE_NULL && *FormatString != L'}'; Size++)
            FormatString++;

        /* Skip '}', '\r', '\n' */
        FormatString += 3;

        TempStringU.Length = (USHORT)(Size * sizeof(WCHAR));
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
            RtlInitEmptyUnicodeString(&TempStringU, NULL, 0);
    }

    /* Retrieve the window title of the client, if it has one */
    RtlInitEmptyUnicodeString(&WindowTitleU, NULL, 0);
    hwndOwner = NULL;
    EnumThreadWindows(HandleToUlong(Message->h.ClientId.UniqueThread),
                      FindTopLevelWnd, (LPARAM)&hwndOwner);
    if (hwndOwner)
    {
        cchBuffer = GetWindowTextLengthW(hwndOwner);
        if (cchBuffer != 0)
        {
            cchBuffer += 3; // 2 characters for ": " and a NULL terminator.
            WindowTitleU.MaximumLength = (USHORT)(cchBuffer * sizeof(WCHAR));
            WindowTitleU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  HEAP_ZERO_MEMORY,
                                                  WindowTitleU.MaximumLength);
            if (WindowTitleU.Buffer)
            {
                cchBuffer = GetWindowTextW(hwndOwner,
                                           WindowTitleU.Buffer,
                                           WindowTitleU.MaximumLength / sizeof(WCHAR));
                WindowTitleU.Length = (USHORT)(cchBuffer * sizeof(WCHAR));
                RtlAppendUnicodeToString(&WindowTitleU, L": ");
            }
            else
            {
                RtlInitEmptyUnicodeString(&WindowTitleU, NULL, 0);
            }
        }
    }

    /* Calculate buffer length for the caption */
    CaptionStringU->MaximumLength = WindowTitleU.Length +
                                    FileNameU.Length + TempStringU.Length +
                                    3 * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    /* Allocate a buffer for the caption */
    CaptionStringU->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             CaptionStringU->MaximumLength);
    if (!CaptionStringU->Buffer)
    {
        DPRINT1("Cannot allocate memory for CaptionStringU\n");
        Status = STATUS_NO_MEMORY;
    }

    /* Append the file name, the separator and the caption text */
    RtlStringCbPrintfW(CaptionStringU->Buffer,
                       CaptionStringU->MaximumLength,
                       L"%wZ%wZ - %wZ",
                       &WindowTitleU, &FileNameU, &TempStringU);
    CaptionStringU->Length = wcslen(CaptionStringU->Buffer) * sizeof(WCHAR);

    /* Free string buffers if needed */
    if (WindowTitleU.Buffer) RtlFreeUnicodeString(&WindowTitleU);
    if (hProcess) RtlFreeUnicodeString(&FileNameU);

    // FIXME: What is 42 == ??
    Size = 42;

    /* Check if this is an exception message */
    if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
    {
        ExceptionCode = CapturedParameters[0];

        /* Get text string of the exception code */
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
                RtlInitAnsiString(&Format2A, (PCHAR)MessageResource->Text);
                /* Status = */ RtlAnsiStringToUnicodeString(&Format2U, &Format2A, TRUE);
            }

            /* Handle special cases */
            if (ExceptionCode == STATUS_ACCESS_VIOLATION)
            {
                FormatString = Format2U.Buffer;
                CapturedParameters[0] = CapturedParameters[1];
                CapturedParameters[1] = CapturedParameters[3];
                if (CapturedParameters[2])
                    CapturedParameters[2] = (ULONG_PTR)L"written";
                else
                    CapturedParameters[2] = (ULONG_PTR)L"read";
            }
            else if (ExceptionCode == STATUS_IN_PAGE_ERROR)
            {
                FormatString = Format2U.Buffer;
                CapturedParameters[0] = CapturedParameters[1];
                CapturedParameters[1] = CapturedParameters[3];
            }
            else
            {
                /* Keep the existing FormatString */
                CapturedParameters[2] = CapturedParameters[1];
                CapturedParameters[1] = CapturedParameters[0];

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

                    CapturedParameters[0] = (ULONG_PTR)pszBuffer;
                }
                else
                {
                    /* Fall back to hardcoded value */
                    CapturedParameters[0] = (ULONG_PTR)L"unknown software exception";
                }
            }
        }
        else
        {
            /* Fall back to hardcoded value, and keep the existing FormatString */
            CapturedParameters[2] = CapturedParameters[1];
            CapturedParameters[1] = CapturedParameters[0];
            CapturedParameters[0] = (ULONG_PTR)L"unknown software exception";
        }

        if (Message->ValidResponseOptions == OptionOk ||
            Message->ValidResponseOptions == OptionOkCancel)
        {
            /* Reserve space for one newline and the OK-terminate-program string */
            Size += 1 + (g_OKTerminateU.Length / sizeof(WCHAR));
        }
        if (Message->ValidResponseOptions == OptionOkCancel)
        {
            /* Reserve space for one newline and the CANCEL-debug-program string */
            Size += 1 + (g_CancelDebugU.Length / sizeof(WCHAR));
        }
    }

    /* Calculate length of text buffer */
    TextStringU->MaximumLength = FormatU.Length + SizeOfStrings +
                                 (USHORT)(Size * sizeof(WCHAR)) +
                                 sizeof(UNICODE_NULL);

    /* Allocate a buffer for the text */
    TextStringU->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          TextStringU->MaximumLength);
    if (!TextStringU->Buffer)
    {
        DPRINT1("Cannot allocate memory for TextStringU\n");
        Status = STATUS_NO_MEMORY;
    }

    Status = STATUS_SUCCESS;

    /* Wrap in SEH to protect from invalid string parameters */
    _SEH2_TRY
    {
        /* Print the string into the buffer */
        pszBuffer = TextStringU->Buffer;
        cchBuffer = TextStringU->MaximumLength;
        RtlStringCbPrintfExW(pszBuffer, cchBuffer,
                             &pszBuffer, &cchBuffer,
                             STRSAFE_IGNORE_NULLS,
                             FormatString,
                             CapturedParameters[0],
                             CapturedParameters[1],
                             CapturedParameters[2],
                             CapturedParameters[3]);

        if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
        {
            if (Message->ValidResponseOptions == OptionOk ||
                Message->ValidResponseOptions == OptionOkCancel)
            {
                RtlStringCbPrintfExW(pszBuffer, cchBuffer,
                                     &pszBuffer, &cchBuffer,
                                     STRSAFE_IGNORE_NULLS,
                                     L"\n%wZ",
                                     &g_OKTerminateU);
            }
            if (Message->ValidResponseOptions == OptionOkCancel)
            {
                RtlStringCbPrintfExW(pszBuffer, cchBuffer,
                                     &pszBuffer, &cchBuffer,
                                     STRSAFE_IGNORE_NULLS,
                                     L"\n%wZ",
                                     &g_CancelDebugU);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* An exception occurred, use a default string */
        DPRINT1("Exception 0x%08lx occurred while building hard-error message, fall back to default message.\n",
                _SEH2_GetExceptionCode());

        RtlStringCbPrintfW(TextStringU->Buffer,
                           TextStringU->MaximumLength,
                           L"Exception processing message 0x%08lx\n"
                           L"Parameters: 0x%p 0x%p 0x%p 0x%p",
                           Message->Status,
                           CapturedParameters[0], CapturedParameters[1],
                           CapturedParameters[2], CapturedParameters[3]);

        /* Set error and free buffers */
        // Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (NT_SUCCESS(Status))
    {
        TextStringU->Length = wcslen(TextStringU->Buffer) * sizeof(WCHAR);
    }

    /* Free converted Unicode strings */
    if (Format2A.Buffer) RtlFreeUnicodeString(&Format2U);
    if (FormatA.Buffer) RtlFreeUnicodeString(&FormatU);

    return Status;
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
UserpShowInformationBalloon(PWSTR Text,
                            PWSTR Caption,
                            PHARDERROR_MSG Message)
{
    ULONG ShellErrorMode;
    HWND hwnd;
    COPYDATASTRUCT CopyData;
    PBALLOON_HARD_ERROR_DATA pdata;
    DWORD dwSize, cbTextLen, cbTitleLen;
    PWCHAR pText, pCaption;
    DWORD ret, dwResult;

    /* Query the shell error mode value */
    ShellErrorMode = GetRegInt(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Windows",
                               L"ShellErrorMode", 0);

    /* Make the shell display the hard error message in balloon only if necessary */
    if (ShellErrorMode != 1)
        return FALSE;

    hwnd = GetTaskmanWindow();
    if (!hwnd)
    {
        DPRINT1("Failed to find shell task window (last error %lu)\n", GetLastError());
        return FALSE;
    }

    cbTextLen  = ((Text    ? wcslen(Text)    : 0) + 1) * sizeof(WCHAR);
    cbTitleLen = ((Caption ? wcslen(Caption) : 0) + 1) * sizeof(WCHAR);

    dwSize = sizeof(BALLOON_HARD_ERROR_DATA);
    dwSize += cbTextLen + cbTitleLen;

    pdata = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!pdata)
    {
        DPRINT1("Failed to allocate balloon data\n");
        return FALSE;
    }

    pdata->cbHeaderSize = sizeof(BALLOON_HARD_ERROR_DATA);
    pdata->Status = Message->Status;

    if (NT_SUCCESS(Message->Status))
        pdata->dwType = MB_OK;
    else if (Message->Status == STATUS_SERVICE_NOTIFICATION)
        pdata->dwType = Message->Parameters[2];
    else
        pdata->dwType = MB_ICONINFORMATION;

    pdata->TitleOffset = pdata->cbHeaderSize;
    pdata->MessageOffset = pdata->TitleOffset;
    pdata->MessageOffset += cbTitleLen;
    pCaption = (PWCHAR)((ULONG_PTR)pdata + pdata->TitleOffset);
    pText = (PWCHAR)((ULONG_PTR)pdata + pdata->MessageOffset);
    wcscpy(pCaption, Caption);
    wcscpy(pText, Text);

    CopyData.dwData = RegisterWindowMessageW(L"HardError");
    CopyData.cbData = dwSize;
    CopyData.lpData = pdata;

    dwResult = FALSE;

    ret = SendMessageTimeoutW(hwnd, WM_COPYDATA, 0, (LPARAM)&CopyData,
                              SMTO_NORMAL | SMTO_ABORTIFHUNG, 3000, &dwResult);

    RtlFreeHeap(RtlGetProcessHeap(), 0, pdata);

    return (ret && dwResult) ? TRUE : FALSE;
}

static
ULONG
UserpMessageBox(
    IN PCWSTR Text,
    IN PCWSTR Caption,
    IN ULONG ValidResponseOptions,
    IN ULONG Severity,
    IN ULONG Timeout)
{
    ULONG Type, MessageBoxResponse;

    /* Set the message box type */
    switch (ValidResponseOptions)
    {
        case OptionAbortRetryIgnore:
            Type = MB_ABORTRETRYIGNORE;
            break;
        case OptionOk:
            Type = MB_OK;
            break;
        case OptionOkCancel:
            Type = MB_OKCANCEL;
            break;
        case OptionRetryCancel:
            Type = MB_RETRYCANCEL;
            break;
        case OptionYesNo:
            Type = MB_YESNO;
            break;
        case OptionYesNoCancel:
            Type = MB_YESNOCANCEL;
            break;
        case OptionShutdownSystem:
            Type = MB_RETRYCANCEL; // FIXME???
            break;
        case OptionOkNoWait:
            /*
             * At that point showing the balloon failed. Is that correct?
             */
            Type = MB_OK; // FIXME!
            break;
        case OptionCancelTryContinue:
            Type = MB_CANCELTRYCONTINUE;
            break;

        /* Anything else is invalid */
        default:
        {
            DPRINT1("Unknown ValidResponseOptions = %d\n", ValidResponseOptions);
            return ResponseNotHandled;
        }
    }

    /* Set severity */
    // STATUS_SEVERITY_SUCCESS
         if (Severity == STATUS_SEVERITY_INFORMATIONAL) Type |= MB_ICONINFORMATION;
    else if (Severity == STATUS_SEVERITY_WARNING)       Type |= MB_ICONWARNING;
    else if (Severity == STATUS_SEVERITY_ERROR)         Type |= MB_ICONERROR;

    Type |= MB_SYSTEMMODAL | MB_SETFOREGROUND;

    DPRINT("Text = '%S', Caption = '%S', Severity = %d, Type = 0x%lx\n",
           Text, Caption, Severity, Type);

    /* Display a message box */
    MessageBoxResponse = MessageBoxTimeoutW(NULL, Text, Caption, Type, 0, Timeout);

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
    ULONG_PTR Parameters[MAXIMUM_HARDERROR_PARAMETERS] = {0};
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING TextU, CaptionU;
    NTSTATUS Status;
    HANDLE hProcess;
    ULONG Size;
    ULONG ErrorMode;

    ASSERT(ThreadData->Process != NULL);

    /* Default to not handled */
    Message->Response = ResponseNotHandled;

    /* Make sure we don't have too many parameters */
    if (Message->NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
    {
        // FIXME: Windows just fails (STATUS_INVALID_PARAMETER) & returns ResponseNotHandled.
        Message->NumberOfParameters = MAXIMUM_HARDERROR_PARAMETERS;
    }
    if (Message->ValidResponseOptions > OptionCancelTryContinue)
    {
        // STATUS_INVALID_PARAMETER;
        Message->Response = ResponseNotHandled;
        return;
    }
    // TODO: More message validation: check NumberOfParameters wrt. Message Status code.

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

    /* Re-initialize the hard errors cache */
    UserInitHardErrorsCache();

    /* Capture all string parameters from the process memory */
    UserpCaptureStringParameters(Parameters, &Size, Message, hProcess);

    /* Format the message caption and text */
    Status = UserpFormatMessages(&TextU,
                                 &CaptionU,
                                 Parameters,
                                 Size,
                                 Message,
                                 hProcess);

    /* Cleanup */
    UserpFreeStringParameters(Parameters, Message);
    if (hProcess) NtClose(hProcess);

    /* If we failed, bail out */
    if (!NT_SUCCESS(Status))
        return;

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
        if (UserpShowInformationBalloon(TextU.Buffer,
                                        CaptionU.Buffer,
                                        Message))
        {
            Message->Response = ResponseOk;
            goto Quit;
        }
    }

    /* Display the message box */
    Message->Response = UserpMessageBox(TextU.Buffer,
                                        CaptionU.Buffer,
                                        Message->ValidResponseOptions,
                                        (ULONG)(Message->Status) >> 30,
                                        (ULONG)-1);

Quit:
    RtlFreeUnicodeString(&TextU);
    RtlFreeUnicodeString(&CaptionU);

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
