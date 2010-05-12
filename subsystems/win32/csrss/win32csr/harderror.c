/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>
#include <strsafe.h>

#define IDTRYAGAIN 10
#define IDCONTINUE 11

/* FUNCTIONS *****************************************************************/

static
NTSTATUS
CsrpGetClientFileName(
    OUT PUNICODE_STRING ClientFileNameU,
    HANDLE hProcess)
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
    ClientFileNameU->MaximumLength = 0;
    ClientFileNameU->Length = 0;
    ClientFileNameU->Buffer = NULL;

    /* Query process information */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ClientBasicInfo,
                                       sizeof(ClientBasicInfo),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

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

    Status = NtReadVirtualMemory(hProcess,
                                 ModuleData.BaseDllName.Buffer,
                                 ClientFileNameU->Buffer,
                                 ClientFileNameU->MaximumLength,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ClientFileNameU->Buffer);
        ClientFileNameU->Buffer = NULL;
        ClientFileNameU->MaximumLength = 0;
        return Status;
    }

    ClientFileNameU->Length = wcslen(ClientFileNameU->Buffer)*sizeof(wchar_t);
    DPRINT("ClientFileNameU=\'%wZ\'\n", &ClientFileNameU);

    return STATUS_SUCCESS;
}


static
NTSTATUS
CsrpCaptureStringParameters(
    OUT PULONG_PTR Parameters,
    OUT PULONG SizeOfAllUnicodeStrings,
    IN PHARDERROR_MSG HardErrorMessage,
    HANDLE hProcess)
{
    ULONG nParam, UnicodeStringParameterMask, Size = 0;
    NTSTATUS Status;
    UNICODE_STRING TempStringU;
    PWSTR ParamString;

    UnicodeStringParameterMask = HardErrorMessage->UnicodeStringParameterMask;

    /* Read all strings from client space */
    for (nParam = 0; 
         nParam < HardErrorMessage->NumberOfParameters; 
         nParam++, UnicodeStringParameterMask >>= 1)
    {
        Parameters[nParam] = 0;

        /* Check if the current parameter is a unicode string */
        if (UnicodeStringParameterMask & 0x01)
        {
            /* Read the UNICODE_STRING from the process memory */
            Status = NtReadVirtualMemory(hProcess,
                                         (PVOID)HardErrorMessage->Parameters[nParam],
                                         &TempStringU,
                                         sizeof(TempStringU),
                                         NULL);

            if (!NT_SUCCESS(Status)) return Status;

            /* Allocate a buffer for the string */
            ParamString = RtlAllocateHeap(RtlGetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          TempStringU.Length + sizeof(WCHAR));

            if (!ParamString)
            {
                DPRINT1("Cannot allocate memory %d\n", TempStringU.Length);
                return STATUS_NO_MEMORY;
            }

            /* Read the string buffer from the process memory */
            Status = NtReadVirtualMemory(hProcess,
                                         TempStringU.Buffer,
                                         ParamString,
                                         TempStringU.Length,
                                         NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtReadVirtualMemory failed with code: %lx\n", Status);
                RtlFreeHeap(RtlGetProcessHeap(), 0, ParamString);
                return Status;
            }

            /* Zero terminate the string */
            ParamString[TempStringU.Length / sizeof(WCHAR)] = 0;
            DPRINT("ParamString=\'%S\'\n", ParamString);

            Parameters[nParam] = (ULONG_PTR)ParamString;
            Size += TempStringU.Length;
        }
        else
        {
            /* It's not a unicode string */
            Parameters[nParam] = HardErrorMessage->Parameters[nParam];
        }
    }

    *SizeOfAllUnicodeStrings = Size;
    return STATUS_SUCCESS;
}

static
VOID
CsrpFreeStringParameters(
    IN OUT PULONG_PTR Parameters,
    IN PHARDERROR_MSG HardErrorMessage)
{
    ULONG nParam, UnicodeStringParameterMask;

    UnicodeStringParameterMask = HardErrorMessage->UnicodeStringParameterMask;

    /* Loop all parameters */
    for (nParam = 0; 
         nParam < HardErrorMessage->NumberOfParameters; 
         nParam++, UnicodeStringParameterMask >>= 1)
    {
        /* Check if the current parameter is a string */
        if (UnicodeStringParameterMask & 0x01)
        {
            /* Free the string buffer */
            RtlFreeHeap(RtlGetProcessHeap(), 0, (PVOID)Parameters[nParam]);
        }
    }
}


static
NTSTATUS
CsrpFormatMessages(
    OUT PUNICODE_STRING TextStringU,
    OUT PUNICODE_STRING CaptionStringU,
    IN  PULONG_PTR Parameters,
    IN  ULONG SizeOfStrings,
    IN  PHARDERROR_MSG Message,
    IN  HANDLE hProcess)
{
    NTSTATUS Status;
    UNICODE_STRING FileNameU, TempStringU, FormatU;
    ANSI_STRING FormatA;
    PRTL_MESSAGE_RESOURCE_ENTRY MessageResource;
    PWSTR FormatString;
    ULONG Size;

    /* Get the file name of the client process */
    CsrpGetClientFileName(&FileNameU, hProcess);

    /* Check if we have a file name */
    if (!FileNameU.Buffer)
    {
        /* No, use system */
        RtlInitUnicodeString(&FileNameU, L"System");
    }

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
            RtlInitAnsiString(&FormatA, MessageResource->Text);
            RtlAnsiStringToUnicodeString(&FormatU, &FormatA, TRUE);
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
        for (Size = 0; *FormatString != 0 && *FormatString != L'}'; Size++)
            FormatString++;

        /* Skip '}', '\r', '\n' */
        FormatString += 3;

        TempStringU.Length = Size * sizeof(WCHAR);
        TempStringU.MaximumLength = TempStringU.Length;
    }
    else
    {
        /* FIXME: Set string based on severity */
        RtlInitUnicodeString(&TempStringU, L"Application Error");
    }

    /* Calculate buffer length for the caption */
    CaptionStringU->MaximumLength = FileNameU.Length + TempStringU.Length +
                                    4 * sizeof(WCHAR);

    /* Allocate a buffer for the caption */
    CaptionStringU->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             CaptionStringU->MaximumLength);

    /* Append the file name, seperator and the caption text */
    CaptionStringU->Length = 0;
    RtlAppendUnicodeStringToString(CaptionStringU, &FileNameU);
    RtlAppendUnicodeToString(CaptionStringU, L" - ");
    RtlAppendUnicodeStringToString(CaptionStringU, &TempStringU);

    /* Zero terminate the buffer */
    CaptionStringU->Buffer[CaptionStringU->Length] = 0;

    /* Free the file name buffer */
    RtlFreeUnicodeString(&FileNameU);

    /* Check if this is an exception message */
    if (Message->Status == STATUS_UNHANDLED_EXCEPTION)
    {
        /* Handle special cases */
        if (Parameters[0] == STATUS_ACCESS_VIOLATION)
        {
            Parameters[0] = Parameters[1];
            Parameters[1] = Parameters[3];
            if (Parameters[2]) Parameters[2] = (ULONG_PTR)L"written";
            else Parameters[2] = (ULONG_PTR)L"read";
            MessageResource = NULL;
        }
        else if (Parameters[0] == STATUS_IN_PAGE_ERROR)
        {
            Parameters[0] = Parameters[1];
            Parameters[1] = Parameters[3];
            MessageResource = NULL;
        }
        else
        {
            /* Fall back to hardcoded value */
            Parameters[2] = Parameters[1];
            Parameters[1] = Parameters[0];
            Parameters[0] = (ULONG_PTR)L"unknown software exception";
        }

        if (!MessageResource)
        {
            /* Get text string of the exception code */
            Status = RtlFindMessage(GetModuleHandleW(L"ntdll"),
                                    (ULONG_PTR)RT_MESSAGETABLE,
                                    LANG_NEUTRAL,
                                    Parameters[0],
                                    &MessageResource);

            if (NT_SUCCESS(Status))
            {
                if (FormatA.Buffer) RtlFreeUnicodeString(&FormatU);

                if (MessageResource->Flags)
                {
                    RtlInitUnicodeString(&FormatU, (PWSTR)MessageResource->Text);
                    FormatA.Buffer = NULL;
                }
                else
                {
                    RtlInitAnsiString(&FormatA, MessageResource->Text);
                    RtlAnsiStringToUnicodeString(&FormatU, &FormatA, TRUE);
                }
            }
            else
            {
                /* Fall back to hardcoded value */
                Parameters[2] = Parameters[1];
                Parameters[1] = Parameters[0];
                Parameters[0] = (ULONG_PTR)L"unknown software exception";
            }
        }
    }

    /* Calculate length of text buffer */
    TextStringU->MaximumLength = wcslen(FormatString) * sizeof(WCHAR) +
                                     SizeOfStrings + 42 * sizeof(WCHAR);
    
    /* Allocate a buffer for the text */
    TextStringU->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          TextStringU->MaximumLength);

    /* Wrap in SEH to protect from invalid string parameters */
    _SEH2_TRY
    {
        /* Print the string into the buffer */
        StringCbPrintfW(TextStringU->Buffer,
                        TextStringU->MaximumLength,
                        FormatString,
                        Parameters[0],
                        Parameters[1],
                        Parameters[2],
                        Parameters[3],
                        Parameters[4]);
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set error and free buffers */
        Status = _SEH2_GetExceptionCode();
        RtlFreeHeap(RtlGetProcessHeap(), 0, TextStringU->Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CaptionStringU->Buffer);
    }
    _SEH2_END

    if (NT_SUCCESS(Status))
    {
        TextStringU->Length = wcslen(TextStringU->Buffer) * sizeof(WCHAR);
    }

    if (FormatA.Buffer) RtlFreeUnicodeString(&FormatU);

    return Status;
}

static
ULONG
CsrpMessageBox(
    PWSTR Text,
    PWSTR Caption,
    ULONG ValidResponseOptions,
    ULONG Severity)
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
        /* Anything else is invalid */
        default:
            return ResponseNotHandled;
    }

    /* Set severity */
    if (Severity == STATUS_SEVERITY_INFORMATIONAL) Type |= MB_ICONINFORMATION;
    else if (Severity == STATUS_SEVERITY_WARNING) Type |= MB_ICONWARNING;
    else if (Severity == STATUS_SEVERITY_ERROR) Type |= MB_ICONERROR;

    Type |= MB_SYSTEMMODAL | MB_SETFOREGROUND;

    DPRINT("Text = '%S', Caption = '%S', Severity = %d, Type = 0x%lx\n", 
           Text, Caption, Severity, Type);

    /* Display a message box */
    MessageBoxResponse = MessageBoxW(0, Text, Caption, Type);

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

BOOL
WINAPI
Win32CsrHardError(
    IN PCSRSS_PROCESS_DATA ProcessData,
    IN PHARDERROR_MSG Message)
{
    ULONG_PTR Parameters[MAXIMUM_HARDERROR_PARAMETERS];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING TextU, CaptionU;
    NTSTATUS Status;
    HANDLE hProcess;
    ULONG Size;

    /* Default to not handled */
    Message->Response = ResponseNotHandled;

    /* Make sure we don't have too many parameters */
    if (Message->NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS)
        Message->NumberOfParameters = MAXIMUM_HARDERROR_PARAMETERS;

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    /* Open client process */
    Status = NtOpenProcess(&hProcess,
                           PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &Message->h.ClientId);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenProcess failed with code: %lx\n", Status);
        return FALSE;
    }

    /* Capture all string parameters from the process memory */
    Status = CsrpCaptureStringParameters(Parameters, &Size, Message, hProcess);
    if (!NT_SUCCESS(Status))
    {
        NtClose(hProcess);
        return FALSE;
    }

    /* Format the caption and message box text */
    Status = CsrpFormatMessages(&TextU,
                                &CaptionU,
                                Parameters,
                                Size,
                                Message,
                                hProcess);

    /* Cleanup */
    CsrpFreeStringParameters(Parameters, Message);
    NtClose(hProcess);

    if (!NT_SUCCESS(Status))
    {
       return FALSE;
    }

    /* Display the message box */
    Message->Response = CsrpMessageBox(TextU.Buffer,
                                       CaptionU.Buffer,
                                       Message->ValidResponseOptions,
                                       (ULONG)Message->Status >> 30);

    RtlFreeUnicodeString(&TextU);
    RtlFreeUnicodeString(&CaptionU);

    return TRUE;
}

