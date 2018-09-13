#define SE32To64(x32) ((ULONG64)(LONG64)(LONG)(x32))
#define SEPtrTo64(x32) ((ULONG64)(LONG64)(LONG_PTR)(x32))

__inline
BOOL
Is64PtrSE(
    ULONG64 p64
    )
/*++
Description:
    Is the pointer sign extended? It is an error if the pointer 
    is in the range of 2GB and 4GB
--*/
{
    return ((p64 < 0x0000000080000000) || (0x100000000 <= p64));
}





#ifndef WINDBG_POINTERS_MACROS_ONLY

__inline
void
String32To64(
    PSTRING32 p32,
    PSTRING64 p64
    )
{
    p64->Length = p32->Length;
    p64->MaximumLength = p32->MaximumLength;
    COPYSE(p64, p32, Buffer);
}

__inline
void
AnsiString32To64(
    PANSI_STRING32 p32,
    PANSI_STRING64 p64
    )
{
    p64->Length = p32->Length;
    p64->MaximumLength = p32->MaximumLength;
    COPYSE(p64, p32, Buffer);
}

__inline
void
UnicodeString32To64(
    PUNICODE_STRING32 p32,
    PUNICODE_STRING64 p64
    )
{
    p64->Length = p32->Length;
    p64->MaximumLength = p32->MaximumLength;
    COPYSE(p64, p32, Buffer);
}

typedef struct _EXCEPTION_DEBUG_INFO32 {
    EXCEPTION_RECORD32 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO32, *LPEXCEPTION_DEBUG_INFO32;

typedef struct _EXCEPTION_DEBUG_INFO64 {
    EXCEPTION_RECORD64 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO64, *LPEXCEPTION_DEBUG_INFO64;

__inline
VOID
ExceptionDebugInfo32To64(
    LPEXCEPTION_DEBUG_INFO32 e32,
    LPEXCEPTION_DEBUG_INFO64 e64
    )
{
    ExceptionRecord32To64(&e32->ExceptionRecord, &e64->ExceptionRecord);
    e64->dwFirstChance = e32->dwFirstChance;
}

__inline
VOID
ExceptionDebugInfo64To32(
    LPEXCEPTION_DEBUG_INFO64 e64,
    LPEXCEPTION_DEBUG_INFO32 e32
    )
{
    ExceptionRecord64To32(&e64->ExceptionRecord, &e32->ExceptionRecord);
    e32->dwFirstChance = e64->dwFirstChance;
}

typedef struct _CREATE_THREAD_DEBUG_INFO32 {
    HANDLE hThread;
    DWORD lpThreadLocalBase;
    DWORD lpStartAddress;
} CREATE_THREAD_DEBUG_INFO32, *LPCREATE_THREAD_DEBUG_INFO32;

typedef struct _CREATE_THREAD_DEBUG_INFO64 {
    HANDLE hThread;
    DWORD64 lpThreadLocalBase;
    DWORD64 lpStartAddress;
} CREATE_THREAD_DEBUG_INFO64, *LPCREATE_THREAD_DEBUG_INFO64;

__inline
VOID
CreateThreadDebugInfo32To64(
    LPCREATE_THREAD_DEBUG_INFO32 c32,
    LPCREATE_THREAD_DEBUG_INFO64 c64
    )
{
    c64->hThread = c32->hThread;
    c64->lpThreadLocalBase = SE32To64( c32->lpThreadLocalBase );
    c64->lpStartAddress = SE32To64( c32->lpStartAddress );
}

__inline
VOID
CreateThreadDebugInfo64To32(
    LPCREATE_THREAD_DEBUG_INFO64 c64,
    LPCREATE_THREAD_DEBUG_INFO32 c32
    )
{
    c32->hThread = c64->hThread;
    c32->lpThreadLocalBase = (DWORD)c64->lpThreadLocalBase;
    c32->lpStartAddress = (DWORD)c64->lpStartAddress;
}

typedef struct _CREATE_PROCESS_DEBUG_INFO32 {
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    DWORD lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    DWORD lpThreadLocalBase;
    DWORD lpStartAddress;
    DWORD lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO32, *LPCREATE_PROCESS_DEBUG_INFO32;

typedef struct _CREATE_PROCESS_DEBUG_INFO64 {
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    DWORD64 lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    DWORD64 lpThreadLocalBase;
    DWORD64 lpStartAddress;
    DWORD64 lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO64, *LPCREATE_PROCESS_DEBUG_INFO64;

__inline
VOID
CreateProcessDebugInfo32To64(
    LPCREATE_PROCESS_DEBUG_INFO32 c32,
    LPCREATE_PROCESS_DEBUG_INFO64 c64
    )
{
    c64->hFile = c32->hFile;
    c64->hProcess = c32->hProcess;
    c64->hThread = c32->hThread;
    c64->lpBaseOfImage = SE32To64( c32->lpBaseOfImage );
    c64->dwDebugInfoFileOffset = c32->dwDebugInfoFileOffset;
    c64->nDebugInfoSize = c32->nDebugInfoSize;
    c64->lpThreadLocalBase = SE32To64( c32->lpThreadLocalBase );
    c64->lpStartAddress = SE32To64( c32->lpStartAddress );
    c64->lpImageName = SE32To64( c32->lpImageName );
    c64->fUnicode = c32->fUnicode;
}

__inline
VOID
CreateProcessDebugInfo64To32(
    LPCREATE_PROCESS_DEBUG_INFO64 c64,
    LPCREATE_PROCESS_DEBUG_INFO32 c32
    )
{
    c32->hFile = c64->hFile;
    c32->hProcess = c64->hProcess;
    c32->hThread = c64->hThread;
    c32->lpBaseOfImage = (DWORD)c64->lpBaseOfImage;
    c32->dwDebugInfoFileOffset = c64->dwDebugInfoFileOffset;
    c32->nDebugInfoSize = c64->nDebugInfoSize;
    c32->lpThreadLocalBase = (DWORD)c64->lpThreadLocalBase;
    c32->lpStartAddress = (DWORD)c64->lpStartAddress;
    c32->lpImageName = (DWORD)c64->lpImageName;
    c32->fUnicode = c64->fUnicode;
}

typedef struct _EXIT_THREAD_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO32, *LPEXIT_THREAD_DEBUG_INFO32;

typedef struct _EXIT_THREAD_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO64, *LPEXIT_THREAD_DEBUG_INFO64;

__inline
VOID
ExitThreadDebugInfo32To64(
    LPEXIT_THREAD_DEBUG_INFO32 e32,
    LPEXIT_THREAD_DEBUG_INFO64 e64
    )
{
    e64->dwExitCode = e32->dwExitCode;
}

__inline
VOID
ExitThreadDebugInfo64To32(
    LPEXIT_THREAD_DEBUG_INFO64 e64,
    LPEXIT_THREAD_DEBUG_INFO32 e32
    )
{
    e32->dwExitCode = e64->dwExitCode;
}

typedef struct _EXIT_PROCESS_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO32, *LPEXIT_PROCESS_DEBUG_INFO32;

typedef struct _EXIT_PROCESS_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO64, *LPEXIT_PROCESS_DEBUG_INFO64;

__inline
VOID
ExitProcessDebugInfo32To64(
    LPEXIT_PROCESS_DEBUG_INFO32 e32,
    LPEXIT_PROCESS_DEBUG_INFO64 e64
    )
{
    e64->dwExitCode = e32->dwExitCode;
}

__inline
VOID
ExitProcessDebugInfo64To32(
    LPEXIT_PROCESS_DEBUG_INFO64 e64,
    LPEXIT_PROCESS_DEBUG_INFO32 e32
    )
{
    e32->dwExitCode = e64->dwExitCode;
}

typedef struct _LOAD_DLL_DEBUG_INFO32 {
    HANDLE hFile;
    DWORD lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    DWORD lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO32, *LPLOAD_DLL_DEBUG_INFO32;

typedef struct _LOAD_DLL_DEBUG_INFO64 {
    HANDLE hFile;
    DWORD64 lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    DWORD64 lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO64, *LPLOAD_DLL_DEBUG_INFO64;

__inline
VOID
LoadDllDebugInfo32To64(
    LPLOAD_DLL_DEBUG_INFO32 l32,
    LPLOAD_DLL_DEBUG_INFO64 l64
    )
{
    l64->hFile = l32->hFile;
    l64->lpBaseOfDll = SE32To64( l32->lpBaseOfDll );
    l64->dwDebugInfoFileOffset = l32->dwDebugInfoFileOffset;
    l64->nDebugInfoSize = l32->nDebugInfoSize;
    l64->lpImageName = SE32To64(l32->lpImageName );
    l64->fUnicode = l32->fUnicode;
}

__inline
VOID
LoadDllDebugInfo64To32(
    LPLOAD_DLL_DEBUG_INFO64 l64,
    LPLOAD_DLL_DEBUG_INFO32 l32
    )
{
    l32->hFile = l64->hFile;
    l32->lpBaseOfDll = (DWORD)l64->lpBaseOfDll;
    l32->dwDebugInfoFileOffset = l64->dwDebugInfoFileOffset;
    l32->nDebugInfoSize = l64->nDebugInfoSize;
    l32->lpImageName = (DWORD)l64->lpImageName;
    l32->fUnicode = l64->fUnicode;
}

typedef struct _UNLOAD_DLL_DEBUG_INFO32 {
    DWORD lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO32, *LPUNLOAD_DLL_DEBUG_INFO32;

typedef struct _UNLOAD_DLL_DEBUG_INFO64 {
    DWORD64 lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO64, *LPUNLOAD_DLL_DEBUG_INFO64;

__inline
VOID
UnloadDllDebugInfo32To64(
    LPUNLOAD_DLL_DEBUG_INFO32 u32,
    LPUNLOAD_DLL_DEBUG_INFO64 u64
    )
{
    u64->lpBaseOfDll = SE32To64(u32->lpBaseOfDll);
}

__inline
VOID
UnloadDllDebugInfo64To32(
    LPUNLOAD_DLL_DEBUG_INFO64 u64,
    LPUNLOAD_DLL_DEBUG_INFO32 u32
    )
{
    u32->lpBaseOfDll = (DWORD)u64->lpBaseOfDll;
}

typedef struct _OUTPUT_DEBUG_STRING_INFO32 {
    DWORD lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO32, *LPOUTPUT_DEBUG_STRING_INFO32;

typedef struct _OUTPUT_DEBUG_STRING_INFO64 {
    DWORD64 lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO64, *LPOUTPUT_DEBUG_STRING_INFO64;

__inline
VOID
OutputDebugStringInfo32To64(
    LPOUTPUT_DEBUG_STRING_INFO32 o32,
    LPOUTPUT_DEBUG_STRING_INFO64 o64
    )
{
    o64->lpDebugStringData = SE32To64( o32->lpDebugStringData );
    o64->fUnicode = o32->fUnicode;
    o64->nDebugStringLength = o32->nDebugStringLength;
}

__inline
VOID
OutputDebugStringInfo64To32(
    LPOUTPUT_DEBUG_STRING_INFO64 o64,
    LPOUTPUT_DEBUG_STRING_INFO32 o32
    )
{
    o32->lpDebugStringData = (DWORD)o64->lpDebugStringData;
    o32->fUnicode = o64->fUnicode;
    o32->nDebugStringLength = o64->nDebugStringLength;
}

typedef struct _RIP_INFO32 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO32, *LPRIP_INFO32;

typedef struct _RIP_INFO64 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO64, *LPRIP_INFO64;

__inline
VOID
RipInfo32To64(
    LPRIP_INFO32 r32,
    LPRIP_INFO64 r64
    )
{
    r64->dwError = r32->dwType;
    r64->dwError = r32->dwError;
}

__inline
VOID
RipInfo64To32(
    LPRIP_INFO64 r64,
    LPRIP_INFO32 r32
    )
{
    r32->dwError = r64->dwType;
    r32->dwError = r64->dwError;
}


typedef struct _DEBUG_EVENT32 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO32 Exception;
        CREATE_THREAD_DEBUG_INFO32 CreateThread;
        CREATE_PROCESS_DEBUG_INFO32 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO32 ExitThread;
        EXIT_PROCESS_DEBUG_INFO32 ExitProcess;
        LOAD_DLL_DEBUG_INFO32 LoadDll;
        UNLOAD_DLL_DEBUG_INFO32 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO32 DebugString;
        RIP_INFO32 RipInfo;
    } u;
} DEBUG_EVENT32, *LPDEBUG_EVENT32;

typedef struct _DEBUG_EVENT64 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO64 Exception;
        CREATE_THREAD_DEBUG_INFO64 CreateThread;
        CREATE_PROCESS_DEBUG_INFO64 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO64 ExitThread;
        EXIT_PROCESS_DEBUG_INFO64 ExitProcess;
        LOAD_DLL_DEBUG_INFO64 LoadDll;
        UNLOAD_DLL_DEBUG_INFO64 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO64 DebugString;
        RIP_INFO64 RipInfo;
    } u;
} DEBUG_EVENT64, *LPDEBUG_EVENT64;

__inline
VOID
DebugEvent32To64(
    LPDEBUG_EVENT32 d32,
    LPDEBUG_EVENT64 d64
    )
{
    d64->dwDebugEventCode = d32->dwDebugEventCode;
    d64->dwProcessId = d32->dwProcessId;
    d64->dwThreadId = d32->dwThreadId;

    switch (d32->dwDebugEventCode) {
        case EXCEPTION_DEBUG_EVENT:
            ExceptionDebugInfo32To64(&d32->u.Exception, &d64->u.Exception);
            break;

        case CREATE_THREAD_DEBUG_EVENT:
            CreateThreadDebugInfo32To64(&d32->u.CreateThread, &d64->u.CreateThread);
            break;

        case CREATE_PROCESS_DEBUG_EVENT:
            CreateProcessDebugInfo32To64(&d32->u.CreateProcessInfo, &d64->u.CreateProcessInfo);
            break;

        case EXIT_THREAD_DEBUG_EVENT:
            ExitThreadDebugInfo32To64(&d32->u.ExitThread, &d64->u.ExitThread);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            ExitProcessDebugInfo32To64(&d32->u.ExitProcess, &d64->u.ExitProcess);
            break;

        case LOAD_DLL_DEBUG_EVENT:
            LoadDllDebugInfo32To64(&d32->u.LoadDll, &d64->u.LoadDll);
            break;

        case UNLOAD_DLL_DEBUG_EVENT:
            UnloadDllDebugInfo32To64(&d32->u.UnloadDll, &d64->u.UnloadDll);
            break;

        case OUTPUT_DEBUG_STRING_EVENT:
            OutputDebugStringInfo32To64(&d32->u.DebugString, &d64->u.DebugString);
            break;

        case RIP_EVENT:
            RipInfo32To64(&d32->u.RipInfo, &d64->u.RipInfo);
            break;

#if DBG
        default:
            DebugBreak();
            //ASSERT(0);
            break;
#endif
    }
}

__inline
VOID
DebugEvent64To32(
    LPDEBUG_EVENT64 d64,
    LPDEBUG_EVENT32 d32
    )
{
    d32->dwDebugEventCode = d64->dwDebugEventCode;
    d32->dwProcessId = d64->dwProcessId;
    d32->dwThreadId = d64->dwThreadId;

    switch (d64->dwDebugEventCode) {
        case EXCEPTION_DEBUG_EVENT:
            ExceptionDebugInfo64To32(&d64->u.Exception, &d32->u.Exception);
            break;

        case CREATE_THREAD_DEBUG_EVENT:
            CreateThreadDebugInfo64To32(&d64->u.CreateThread, &d32->u.CreateThread);
            break;

        case CREATE_PROCESS_DEBUG_EVENT:
            CreateProcessDebugInfo64To32(&d64->u.CreateProcessInfo, &d32->u.CreateProcessInfo);
            break;

        case EXIT_THREAD_DEBUG_EVENT:
            ExitThreadDebugInfo64To32(&d64->u.ExitThread, &d32->u.ExitThread);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            ExitProcessDebugInfo64To32(&d64->u.ExitProcess, &d32->u.ExitProcess);
            break;

        case LOAD_DLL_DEBUG_EVENT:
            LoadDllDebugInfo64To32(&d64->u.LoadDll, &d32->u.LoadDll);
            break;

        case UNLOAD_DLL_DEBUG_EVENT:
            UnloadDllDebugInfo64To32(&d64->u.UnloadDll, &d32->u.UnloadDll);
            break;

        case OUTPUT_DEBUG_STRING_EVENT:
            OutputDebugStringInfo64To32(&d64->u.DebugString, &d32->u.DebugString);
            break;

        case RIP_EVENT:
            RipInfo64To32(&d64->u.RipInfo, &d32->u.RipInfo);
            break;

#if DBG
        default:
            DebugBreak();
            //ASSERT(0);
            break;
#endif
    }
}



typedef struct _CURDIR32 {
    UNICODE_STRING32 DosPath;
    HANDLE Handle;
} CURDIR32, *PCURDIR32;

typedef struct _CURDIR64 {
    UNICODE_STRING64 DosPath;
    HANDLE Handle;
} CURDIR64, *PCURDIR64;


__inline
void
Curdir32To64(
    PCURDIR32 p32,
    PCURDIR64 p64
    )
{
    UnicodeString32To64(&p32->DosPath, &p64->DosPath);
    p64->Handle = p32->Handle;
}


typedef struct _RTL_DRIVE_LETTER_CURDIR32 {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING32 DosPath;
} RTL_DRIVE_LETTER_CURDIR32, *PRTL_DRIVE_LETTER_CURDIR32;


typedef struct _RTL_DRIVE_LETTER_CURDIR64 {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING64 DosPath;
} RTL_DRIVE_LETTER_CURDIR64, *PRTL_DRIVE_LETTER_CURDIR64;


__inline
void
DriveLetterCurdir32To64(
    PRTL_DRIVE_LETTER_CURDIR32 p32,
    PRTL_DRIVE_LETTER_CURDIR64 p64
    )
{
    p64->Flags = p32->Flags;
    p64->Length = p32->Length;
    p64->TimeStamp = p32->TimeStamp;
    String32To64(&p32->DosPath, &p64->DosPath);
}


typedef struct _RTL_USER_PROCESS_PARAMETERS32 {
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG  ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR32 CurrentDirectory;        // ProcessParameters
    UNICODE_STRING32 DllPath;         // ProcessParameters
    UNICODE_STRING32 ImagePathName;   // ProcessParameters
    UNICODE_STRING32 CommandLine;     // ProcessParameters
    /*PVOID*/ ULONG Environment;              // NtAllocateVirtualMemory

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING32 WindowTitle;     // ProcessParameters
    UNICODE_STRING32 DesktopInfo;     // ProcessParameters
    UNICODE_STRING32 ShellInfo;       // ProcessParameters
    UNICODE_STRING32 RuntimeData;     // ProcessParameters
    RTL_DRIVE_LETTER_CURDIR32 CurrentDirectores[ RTL_MAX_DRIVE_LETTERS ];
} RTL_USER_PROCESS_PARAMETERS32, *PRTL_USER_PROCESS_PARAMETERS32;

typedef struct _RTL_USER_PROCESS_PARAMETERS64 {
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG  ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR64 CurrentDirectory;        // ProcessParameters
    UNICODE_STRING64 DllPath;         // ProcessParameters
    UNICODE_STRING64 ImagePathName;   // ProcessParameters
    UNICODE_STRING64 CommandLine;     // ProcessParameters
    /*PVOID*/ ULONG64 Environment;              // NtAllocateVirtualMemory

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING64 WindowTitle;     // ProcessParameters
    UNICODE_STRING64 DesktopInfo;     // ProcessParameters
    UNICODE_STRING64 ShellInfo;       // ProcessParameters
    UNICODE_STRING64 RuntimeData;     // ProcessParameters
    RTL_DRIVE_LETTER_CURDIR64 CurrentDirectores[ RTL_MAX_DRIVE_LETTERS ];
} RTL_USER_PROCESS_PARAMETERS64, *PRTL_USER_PROCESS_PARAMETERS64;


__inline
void
UserProcessParameters32To64(
    PRTL_USER_PROCESS_PARAMETERS32 p32,
    PRTL_USER_PROCESS_PARAMETERS64 p64
    )
{
    int i;

    p64->MaximumLength = p32->MaximumLength;
    p64->Length = p32->Length;

    p64->Flags = p32->Flags;
    p64->DebugFlags = p32->DebugFlags;

    p64->ConsoleHandle = p32->ConsoleHandle;
    p64->ConsoleFlags = p32->ConsoleFlags;
    p64->StandardInput = p32->StandardInput;
    p64->StandardOutput = p32->StandardOutput;
    p64->StandardError = p32->StandardError;

    Curdir32To64(&p32->CurrentDirectory, &p64->CurrentDirectory);

    UnicodeString32To64(&p32->DllPath, &p64->DllPath);
    UnicodeString32To64(&p32->ImagePathName, &p64->ImagePathName);
    UnicodeString32To64(&p32->CommandLine, &p64->CommandLine);

    p64->Environment = p32->Environment;              // NtAllocateVirtualMemory

    p64->StartingX = p32->StartingX;
    p64->StartingY = p32->StartingY;
    p64->CountX = p32->CountX;
    p64->CountY = p32->CountY;
    p64->CountCharsX = p32->CountCharsX;
    p64->CountCharsY = p32->CountCharsY;
    p64->FillAttribute = p32->FillAttribute;

    p64->WindowFlags = p32->WindowFlags;
    p64->ShowWindowFlags = p32->ShowWindowFlags;

    UnicodeString32To64(&p32->WindowTitle, &p64->WindowTitle);
    UnicodeString32To64(&p32->DesktopInfo, &p64->DesktopInfo);
    UnicodeString32To64(&p32->ShellInfo, &p64->ShellInfo);
    UnicodeString32To64(&p32->RuntimeData, &p64->RuntimeData);

    for (i = 0; i < RTL_MAX_DRIVE_LETTERS; i++) {
        DriveLetterCurdir32To64(&p32->CurrentDirectores[i], &p64->CurrentDirectores[i]);
    }
}

__inline
void
LdrDataTableEntry32To64(
    PLDR_DATA_TABLE_ENTRY32 pLdr32,
    PLDR_DATA_TABLE_ENTRY64 pLdr64
    )
{
    ListEntry32To64(&pLdr32->InLoadOrderLinks, &pLdr64->InLoadOrderLinks);
    ListEntry32To64(&pLdr32->InMemoryOrderLinks, &pLdr64->InMemoryOrderLinks);
    ListEntry32To64(&pLdr32->InInitializationOrderLinks, &pLdr64->InInitializationOrderLinks);
    
    COPYSE(pLdr64, pLdr32, DllBase);
    COPYSE(pLdr64, pLdr32, EntryPoint);
    
    pLdr64->SizeOfImage = pLdr32->SizeOfImage;
    
    UnicodeString32To64(&pLdr32->FullDllName, &pLdr64->FullDllName);
    UnicodeString32To64(&pLdr32->BaseDllName, &pLdr64->BaseDllName);
    
    pLdr64->Flags = pLdr32->Flags;
    pLdr64->LoadCount = pLdr32->LoadCount;
    pLdr64->TlsIndex = pLdr32->TlsIndex;
    
    ListEntry32To64(&pLdr32->HashLinks, &pLdr64->HashLinks);
    
    COPYSE(pLdr64, pLdr32, LoadedImports);
}


#endif //WINDBG_POINTERS_MACROS_ONLY

