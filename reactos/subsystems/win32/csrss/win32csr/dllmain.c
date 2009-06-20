/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 */

/* INCLUDES ******************************************************************/
#define NDEBUG
#include "w32csr.h"
#include <debug.h>

/* Not defined in any header file */
extern VOID WINAPI PrivateCsrssManualGuiCheck(LONG Check);
extern VOID WINAPI PrivateCsrssInitialized();
extern VOID WINAPI InitializeAppSwitchHook();

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
HINSTANCE Win32CsrDllHandle = NULL;
static CSRSS_EXPORTED_FUNCS CsrExports;

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
  {
    CSRSS_DEFINE_API(WRITE_CONSOLE,                CsrWriteConsole),
    CSRSS_DEFINE_API(READ_CONSOLE,                 CsrReadConsole),
    CSRSS_DEFINE_API(ALLOC_CONSOLE,                CsrAllocConsole),
    CSRSS_DEFINE_API(FREE_CONSOLE,                 CsrFreeConsole),
    CSRSS_DEFINE_API(SCREEN_BUFFER_INFO,           CsrGetScreenBufferInfo),
    CSRSS_DEFINE_API(SET_CURSOR,                   CsrSetCursor),
    CSRSS_DEFINE_API(FILL_OUTPUT,                  CsrFillOutputChar),
    CSRSS_DEFINE_API(READ_INPUT,                   CsrReadInputEvent),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_CHAR,    CsrWriteConsoleOutputChar),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_ATTRIB,  CsrWriteConsoleOutputAttrib),
    CSRSS_DEFINE_API(FILL_OUTPUT_ATTRIB,           CsrFillOutputAttrib),
    CSRSS_DEFINE_API(GET_CURSOR_INFO,              CsrGetCursorInfo),
    CSRSS_DEFINE_API(SET_CURSOR_INFO,              CsrSetCursorInfo),
    CSRSS_DEFINE_API(SET_ATTRIB,                   CsrSetTextAttrib),
    CSRSS_DEFINE_API(GET_CONSOLE_MODE,             CsrGetConsoleMode),
    CSRSS_DEFINE_API(SET_CONSOLE_MODE,             CsrSetConsoleMode),
    CSRSS_DEFINE_API(CREATE_SCREEN_BUFFER,         CsrCreateScreenBuffer),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER,            CsrSetScreenBuffer),
    CSRSS_DEFINE_API(SET_TITLE,                    CsrSetTitle),
    CSRSS_DEFINE_API(GET_TITLE,                    CsrGetTitle),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT,         CsrWriteConsoleOutput),
    CSRSS_DEFINE_API(FLUSH_INPUT_BUFFER,           CsrFlushInputBuffer),
    CSRSS_DEFINE_API(SCROLL_CONSOLE_SCREEN_BUFFER, CsrScrollConsoleScreenBuffer),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_CHAR,     CsrReadConsoleOutputChar),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_ATTRIB,   CsrReadConsoleOutputAttrib),
    CSRSS_DEFINE_API(GET_NUM_INPUT_EVENTS,         CsrGetNumberOfConsoleInputEvents),
    CSRSS_DEFINE_API(EXIT_REACTOS,                 CsrExitReactos),
    CSRSS_DEFINE_API(PEEK_CONSOLE_INPUT,           CsrPeekConsoleInput),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT,          CsrReadConsoleOutput),
    CSRSS_DEFINE_API(WRITE_CONSOLE_INPUT,          CsrWriteConsoleInput),
    CSRSS_DEFINE_API(SETGET_CONSOLE_HW_STATE,      CsrHardwareStateProperty),
    CSRSS_DEFINE_API(GET_CONSOLE_WINDOW,           CsrGetConsoleWindow),
    CSRSS_DEFINE_API(CREATE_DESKTOP,               CsrCreateDesktop),
    CSRSS_DEFINE_API(SHOW_DESKTOP,                 CsrShowDesktop),
    CSRSS_DEFINE_API(HIDE_DESKTOP,                 CsrHideDesktop),
    CSRSS_DEFINE_API(SET_CONSOLE_ICON,             CsrSetConsoleIcon),
    CSRSS_DEFINE_API(SET_LOGON_NOTIFY_WINDOW,      CsrSetLogonNotifyWindow),
    CSRSS_DEFINE_API(REGISTER_LOGON_PROCESS,       CsrRegisterLogonProcess),
    CSRSS_DEFINE_API(GET_CONSOLE_CP,               CsrGetConsoleCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_CP,               CsrSetConsoleCodePage),
    CSRSS_DEFINE_API(GET_CONSOLE_OUTPUT_CP,        CsrGetConsoleOutputCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_OUTPUT_CP,        CsrSetConsoleOutputCodePage),
    CSRSS_DEFINE_API(GET_PROCESS_LIST,             CsrGetProcessList),
    CSRSS_DEFINE_API(ADD_CONSOLE_ALIAS,      CsrAddConsoleAlias),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIAS,      CsrGetConsoleAlias),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES,         CsrGetAllConsoleAliases),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES_LENGTH,  CsrGetAllConsoleAliasesLength),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES,        CsrGetConsoleAliasesExes),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES_LENGTH, CsrGetConsoleAliasesExesLength),
    CSRSS_DEFINE_API(GENERATE_CTRL_EVENT,          CsrGenerateCtrlEvent),
    { 0, 0, NULL }
  };

static CSRSS_OBJECT_DEFINITION Win32CsrObjectDefinitions[] =
  {
    { CONIO_CONSOLE_MAGIC,       ConioDeleteConsole },
    { CONIO_SCREEN_BUFFER_MAGIC, ConioDeleteScreenBuffer },
    { 0,                         NULL }
  };

/* FUNCTIONS *****************************************************************/

BOOL WINAPI
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    {
      Win32CsrDllHandle = hDll;
      InitializeAppSwitchHook();
    }

  return TRUE;
}

NTSTATUS FASTCALL
Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                      PHANDLE Handle,
                      Object_t *Object,
                      DWORD Access,
                      BOOL Inheritable)
{
  return (CsrExports.CsrInsertObjectProc)(ProcessData, Handle, Object, Access, Inheritable);
}

NTSTATUS FASTCALL
Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle,
                 Object_t **Object,
                 DWORD Access)
{
  return (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object, Access);
}

NTSTATUS FASTCALL
Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   DWORD Access,
                   LONG Type)
{
  NTSTATUS Status;

  Status = (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object, Access);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  if ((*Object)->Type != Type)
    {
      (CsrExports.CsrReleaseObjectByPointerProc)(*Object);
      return STATUS_INVALID_HANDLE;
    }

  EnterCriticalSection(&((*Object)->Lock));

  return STATUS_SUCCESS;
}

VOID FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
  LeaveCriticalSection(&(Object->Lock));
  (CsrExports.CsrReleaseObjectByPointerProc)(Object);
}

NTSTATUS FASTCALL
Win32CsrReleaseObjectByPointer(Object_t *Object)
{
  return (CsrExports.CsrReleaseObjectByPointerProc)(Object);
}

NTSTATUS FASTCALL
Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                      HANDLE Object)
{
  return (CsrExports.CsrReleaseObjectProc)(ProcessData, Object);
}

NTSTATUS FASTCALL
Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                      PVOID Context)
{
  return (CsrExports.CsrEnumProcessesProc)(EnumProc, Context);
}

static BOOL WINAPI
Win32CsrInitComplete(void)
{
  PrivateCsrssInitialized();

  return TRUE;
}

static BOOL WINAPI
Win32CsrHardError(IN PCSRSS_PROCESS_DATA ProcessData,
                  IN PHARDERROR_MSG HardErrorMessage)
{
    UINT responce = MB_OK;
    NTSTATUS Status;
    HANDLE hProcess;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG nParam = 0;
    PRTL_MESSAGE_RESOURCE_ENTRY MessageResource;
    ULONG_PTR ParameterList[MAXIMUM_HARDERROR_PARAMETERS];
    LPSTR CaptionText, MessageBody;
    LPWSTR szxCaptionText, szxMessageBody;
    DWORD SizeOfAllUnicodeStrings = 0;
    PROCESS_BASIC_INFORMATION ClientBasicInfo;
    UNICODE_STRING ClientFileNameU;
    UNICODE_STRING TempStringU;
    UNICODE_STRING ParameterStringU;
    ANSI_STRING ParamStringA;
    ULONG UnicodeStringParameterMask = HardErrorMessage->UnicodeStringParameterMask;
    int MessageBoxResponse;

    HardErrorMessage->Response = ResponseNotHandled;

    DPRINT("NumberOfParameters = %d\n", HardErrorMessage->NumberOfParameters);
    DPRINT("Status = %lx\n", HardErrorMessage->Status);

    // open client process
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&hProcess, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &ObjectAttributes, &HardErrorMessage->h.ClientId);
    if( !NT_SUCCESS(Status) ) {
        DPRINT1("NtOpenProcess failed with code: %lx\n", Status);
        return FALSE;
    }

    // let's get a name of the client process to display it in the caption of a message box

    ClientFileNameU.MaximumLength = 0;
    ClientFileNameU.Length = 0;
    ClientFileNameU.Buffer = NULL;
    Status = NtQueryInformationProcess(hProcess, 
        ProcessBasicInformation, 
        &ClientBasicInfo, 
        sizeof(ClientBasicInfo), 
        NULL);
    if( NT_SUCCESS(Status) ) {
        PLIST_ENTRY ModuleListHead;
        PLIST_ENTRY Entry;
        PLDR_DATA_TABLE_ENTRY Module;
        PPEB_LDR_DATA Ldr;
        PPEB Peb = ClientBasicInfo.PebBaseAddress;

        if( Peb )
        {
            Status = NtReadVirtualMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL);
            if( NT_SUCCESS(Status) ) {
                ModuleListHead = &Ldr->InLoadOrderModuleList;
                Status = NtReadVirtualMemory(
                    hProcess,
                    &ModuleListHead->Flink,
                    &Entry,
                    sizeof(Entry),
                    NULL
                    );

                if( NT_SUCCESS(Status) )
                {
                    if (Entry != ModuleListHead)
                    {
                        LDR_DATA_TABLE_ENTRY ModuleData;
                        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

                        Status = NtReadVirtualMemory(hProcess, Module, &ModuleData, sizeof(ModuleData), NULL);
                        if( NT_SUCCESS(Status) ) {
                            PVOID ClientDllBase;

                            Status = NtReadVirtualMemory(
                                hProcess,
                                &Peb->ImageBaseAddress,
                                &ClientDllBase,
                                sizeof(ClientDllBase),
                                NULL
                                );
                            if( NT_SUCCESS(Status) && (ClientDllBase == ModuleData.DllBase) ) {

                                ClientFileNameU.MaximumLength = ModuleData.BaseDllName.MaximumLength;
                                ClientFileNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ClientFileNameU.MaximumLength);
                                Status = NtReadVirtualMemory(
                                    hProcess,
                                    ModuleData.BaseDllName.Buffer,
                                    ClientFileNameU.Buffer,
                                    ClientFileNameU.MaximumLength,
                                    NULL
                                    );
                                if( NT_SUCCESS(Status) ) {
                                    ClientFileNameU.Length = wcslen(ClientFileNameU.Buffer)*sizeof(wchar_t);
                                }
                                else {
                                    RtlFreeHeap (RtlGetProcessHeap(), 0, ClientFileNameU.Buffer);
                                    ClientFileNameU.Buffer = NULL;
                                }

                                DPRINT("ClientFileNameU=\'%wZ\'\n", &ClientFileNameU);
                            }
                        }
                    }
                }
            }
        }
    }

    // read all unicode strings from client space
    for(nParam = 0; nParam < HardErrorMessage->NumberOfParameters; nParam++, UnicodeStringParameterMask >>= 1)
    {
        if( UnicodeStringParameterMask & 0x01 ) {
            Status = NtReadVirtualMemory(hProcess,
                (PVOID)HardErrorMessage->Parameters[nParam],
                (PVOID)&TempStringU, 
                sizeof(TempStringU),
                NULL);

            if( NT_SUCCESS(Status) ) {
                ParameterStringU.Buffer = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, TempStringU.MaximumLength);
                if( !ParameterStringU.Buffer ) {
                    DPRINT1("Cannot allocate memory %d\n", TempStringU.MaximumLength);
                    NtClose(hProcess);
                    if( ClientFileNameU.Buffer ) {
                        RtlFreeHeap (RtlGetProcessHeap(), 0, ClientFileNameU.Buffer);
                    }
                    return FALSE;
                }

                Status = NtReadVirtualMemory(hProcess,
                    (PVOID)TempStringU.Buffer,
                    (PVOID)ParameterStringU.Buffer,
                    TempStringU.MaximumLength,
                    NULL);
                if( !NT_SUCCESS(Status) ) {
                    DPRINT1("NtReadVirtualMemory failed with code: %lx\n", Status);
                    RtlFreeHeap (RtlGetProcessHeap(), 0, ParameterStringU.Buffer);
                    if( ClientFileNameU.Buffer ) {
                        RtlFreeHeap (RtlGetProcessHeap(), 0, ClientFileNameU.Buffer);
                    }
                    NtClose(hProcess);
                    return FALSE;
                }
                ParameterStringU.Length = TempStringU.Length;
                ParameterStringU.MaximumLength = TempStringU.MaximumLength;
                DPRINT("ParameterStringU=\'%wZ\'\n", &ParameterStringU);
                RtlUnicodeStringToAnsiString(&ParamStringA, &ParameterStringU, TRUE);
                ParameterList[nParam] = (ULONG_PTR)ParamStringA.Buffer;
                SizeOfAllUnicodeStrings += ParamStringA.MaximumLength;
            }
        }
        else {
            // it's not a unicode string
            ParameterList[nParam] = HardErrorMessage->Parameters[nParam];
        }
    }

    NtClose(hProcess);

    // get text string of the error code
    Status = RtlFindMessage(
        (PVOID)GetModuleHandle(TEXT("ntdll")),
        (ULONG_PTR)RT_MESSAGETABLE,
        LANG_NEUTRAL,
        HardErrorMessage->Status,
        &MessageResource );
    if( !NT_SUCCESS(Status) ) {
        // WE HAVE TO DISPLAY HERE: "Unknown hard error"
        if( ClientFileNameU.Buffer ) {
            szxCaptionText = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ClientFileNameU.MaximumLength+64);
            wsprintfW(szxCaptionText, L"%s - %hs", ClientFileNameU.Buffer, "Application Error");
        } else {
            szxCaptionText = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 64);
            wsprintfW(szxCaptionText, L"System - Application Error");
        }
        MessageBody = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 38);
        wsprintfA(MessageBody, "Unknown hard error");
    }
    else {
        LPSTR NtStatusString;
        UNICODE_STRING MessageU;
        ANSI_STRING MessageA;
        USHORT CaptionSize = 0;

        if( !MessageResource->Flags ) {
            /* we've got an ansi string */
            DPRINT("MessageResource->Text=%s\n", (PSTR)MessageResource->Text);
            RtlInitAnsiString(&MessageA, MessageResource->Text);
        }
        else {
            /* we've got a unicode string */
            DPRINT("MessageResource->Text=%S\n", (PWSTR)MessageResource->Text);
            RtlInitUnicodeString(&MessageU, (PWSTR)MessageResource->Text);
            RtlUnicodeStringToAnsiString(&MessageA, &MessageU, TRUE);
        }

        // check whether a caption exists
        if( *MessageA.Buffer == '{' ) {
            // get size of the caption
            for( CaptionSize = 0; (CaptionSize < MessageA.Length) && ('}' != MessageA.Buffer[CaptionSize]); CaptionSize++);

            CaptionText = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CaptionSize);
            RtlCopyMemory(CaptionText, MessageA.Buffer+1, CaptionSize-1);
            CaptionSize += 3; // "}\r\n" - 3

            szxCaptionText = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(wchar_t)*CaptionSize+ClientFileNameU.MaximumLength+128);
            if( ClientFileNameU.Buffer ) {
                wsprintfW(szxCaptionText, L"%s - %hs", ClientFileNameU.Buffer, CaptionText);
            } else {
                wsprintfW(szxCaptionText, L"System - %hs", CaptionText);
            }
            RtlFreeHeap (RtlGetProcessHeap(), 0, CaptionText);
        }
        else {
            if( ClientFileNameU.Buffer ) {
                szxCaptionText = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ClientFileNameU.MaximumLength);
                wsprintfW(szxCaptionText, L"%s", ClientFileNameU.Buffer);
            } else {
                szxCaptionText = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 14); // 14 - "System\0\0"
                wsprintfW(szxCaptionText, L"System");
            }
        }
        DPRINT("ParameterList[0]=0x%lx\n", ParameterList[0]);
        if( STATUS_UNHANDLED_EXCEPTION == HardErrorMessage->Status )
        {
            PRTL_MESSAGE_RESOURCE_ENTRY MsgResException;
            MessageBody = NULL;
            Status = RtlFindMessage(
                (PVOID)GetModuleHandle(TEXT("ntdll")),
                (ULONG_PTR)RT_MESSAGETABLE,
                LANG_NEUTRAL,
                ParameterList[0],
                &MsgResException);

            if( NT_SUCCESS(Status) )
            {
                UNICODE_STRING ExcMessageU;
                ANSI_STRING ExcMessageA;
                if( !MsgResException->Flags ) {
                    /* we've got an ansi string */
                    DPRINT("MsgResException->Text=%s\n", (PSTR)MsgResException->Text);
                    RtlInitAnsiString(&ExcMessageA, MsgResException->Text);
                }
                else {
                    /* we've got a unicode string */
                    DPRINT("MsgResException->Text=%S\n", (PWSTR)MsgResException->Text);
                    RtlInitUnicodeString(&ExcMessageU, (PWSTR)MsgResException->Text);
                    RtlUnicodeStringToAnsiString(&ExcMessageA, &ExcMessageU, TRUE);
                }

                MessageBody = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MsgResException->Length+SizeOfAllUnicodeStrings+1024); // 1024 is a magic number I think it should be enough
                if( STATUS_ACCESS_VIOLATION == ParameterList[0] ) {
                    LPSTR pOperationType;
                    if( ParameterList[2] ) pOperationType = "written";
                    else pOperationType = "read";
                    wsprintfA(MessageBody, ExcMessageA.Buffer, ParameterList[1], ParameterList[3], pOperationType);
                }
                else if( STATUS_IN_PAGE_ERROR == ParameterList[0] ) {
                    wsprintfA(MessageBody, ExcMessageA.Buffer, ParameterList[1], ParameterList[3], ParameterList[2]);
                }
            }
            if( !MessageBody ) {
                NtStatusString = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MessageResource->Length-CaptionSize);
                RtlCopyMemory(NtStatusString, MessageA.Buffer+CaptionSize, (MessageResource->Length-CaptionSize)-1);

                MessageBody = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MessageResource->Length+SizeOfAllUnicodeStrings+1024); // 1024 is a magic number I think it should be enough

                wsprintfA(MessageBody, NtStatusString, 
                    L"Unknown software exception",
                    ParameterList[0],
                    ParameterList[1]);

                RtlFreeHeap (RtlGetProcessHeap(), 0, NtStatusString);
            }
        }
        else
        {
            NtStatusString = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MessageResource->Length-CaptionSize);
            RtlCopyMemory(NtStatusString, MessageA.Buffer+CaptionSize, (MessageResource->Length-CaptionSize)-1);

            MessageBody = (LPSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, MessageResource->Length+SizeOfAllUnicodeStrings+1024); // 1024 is a magic number I think it should be enough

            wsprintfA(MessageBody, NtStatusString, 
                ParameterList[0],
                ParameterList[1],
                ParameterList[2],
                ParameterList[3]);

            RtlFreeHeap (RtlGetProcessHeap(), 0, NtStatusString);
        }
        if( MessageResource->Flags ) {
            /* we've got a unicode string */
            RtlFreeAnsiString(&MessageA);
        }
    }
    if( ClientFileNameU.Buffer ) {
        RtlFreeHeap (RtlGetProcessHeap(), 0, ClientFileNameU.Buffer);
    }

    szxMessageBody = (LPWSTR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(wchar_t)*(strlen(MessageBody)+1));
    wsprintfW(szxMessageBody, L"%hs", MessageBody);
    RtlFreeHeap (RtlGetProcessHeap(), 0, MessageBody);

    switch ( HardErrorMessage->ValidResponseOptions )
    {
    case OptionAbortRetryIgnore:
        responce = MB_ABORTRETRYIGNORE;
        break;

    case OptionOk:
        responce = MB_OK;
        break;

    case OptionOkCancel:
        responce = MB_OKCANCEL;
        break;

    case OptionRetryCancel:
        responce = MB_RETRYCANCEL;
        break;

    case OptionYesNo:
        responce = MB_YESNO;
        break;

    case OptionYesNoCancel:
        responce = MB_YESNOCANCEL;
        break;

    case OptionShutdownSystem:
        // XZ??
        break;

    default:
        DPRINT1("Wrong option: ValidResponseOptions = %d\n", HardErrorMessage->ValidResponseOptions);
        ASSERT(FALSE);
        break;
    }

    // FIXME: We should not use MessageBox !!!!
    DPRINT1("%S\n", szxMessageBody);
    MessageBoxResponse = MessageBoxW(0, szxMessageBody, szxCaptionText, responce|MB_ICONERROR|MB_SYSTEMMODAL|MB_SETFOREGROUND);

    RtlFreeHeap (RtlGetProcessHeap(), 0, szxMessageBody);
    RtlFreeHeap (RtlGetProcessHeap(), 0, szxCaptionText);

    switch( MessageBoxResponse )
    {
    case IDOK:
        HardErrorMessage->Response = ResponseOk;
        break;

    case IDCANCEL:
        HardErrorMessage->Response = ResponseCancel;
        break;

    case IDYES:
        HardErrorMessage->Response = ResponseYes;
        break;

    case IDNO:
        HardErrorMessage->Response = ResponseNo;
        break;

    case IDABORT:
        HardErrorMessage->Response = ResponseAbort;
        break;

    case IDIGNORE:
        HardErrorMessage->Response = ResponseIgnore;
        break;

    case IDRETRY:
        HardErrorMessage->Response = ResponseRetry;
        break;

    case 10://IDTRYAGAIN:
        HardErrorMessage->Response = ResponseTryAgain;
        break;

    case 11://IDCONTINUE:
        HardErrorMessage->Response = ResponseContinue;
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return TRUE;
}


BOOL WINAPI
Win32CsrInitialization(PCSRSS_API_DEFINITION *ApiDefinitions,
                       PCSRSS_OBJECT_DEFINITION *ObjectDefinitions,
                       CSRPLUGIN_INIT_COMPLETE_PROC *InitComplete,
                       CSRPLUGIN_HARDERROR_PROC *HardError,
                       PCSRSS_EXPORTED_FUNCS Exports,
                       HANDLE CsrssApiHeap)
{
  NTSTATUS Status;
  CsrExports = *Exports;
  Win32CsrApiHeap = CsrssApiHeap;

  Status = NtUserInitialize(0 ,NULL, NULL);

  PrivateCsrssManualGuiCheck(0);
  CsrInitConsoleSupport();

  *ApiDefinitions = Win32CsrApiDefinitions;
  *ObjectDefinitions = Win32CsrObjectDefinitions;
  *InitComplete = Win32CsrInitComplete;
  *HardError = Win32CsrHardError;

  return TRUE;
}

/* EOF */
