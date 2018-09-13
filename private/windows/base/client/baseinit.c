/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    baseinit.c

Abstract:

    This module implements Win32 base initialization

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"

//
// Divides by 10000
//

ULONG BaseGetTickMagicMultiplier = 10000;
LARGE_INTEGER BaseGetTickMagicDivisor = { 0xd1b71758, 0xe219652c };
CCHAR BaseGetTickMagicShiftCount = 13;
BOOLEAN BaseRunningInServerProcess;
UINT_PTR SystemRangeStart;

#if defined(_WIN64) || defined(BUILD_WOW6432)
SYSTEM_BASIC_INFORMATION SysInfo;
#endif    

BOOLEAN gDoDllRedirection;
WCHAR BaseDefaultPathBuffer[ 2048 ];

//
//  Dispatch functions for Oem/Ansi sensitive conversions
//

NTSTATUS (*Basep8BitStringToUnicodeString)(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    ) = RtlAnsiStringToUnicodeString;

NTSTATUS (*BasepUnicodeStringTo8BitString)(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    ) = RtlUnicodeStringToAnsiString;

ULONG (*BasepUnicodeStringTo8BitSize)(
    PUNICODE_STRING UnicodeString
    ) = BasepUnicodeStringToAnsiSize;

ULONG (*Basep8BitStringToUnicodeSize)(
    PANSI_STRING AnsiString
    ) = BasepAnsiStringToUnicodeSize;


VOID
WINAPI
SetFileApisToOEM(
    VOID
    )
{
    Basep8BitStringToUnicodeString = RtlOemStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToOemString;
    BasepUnicodeStringTo8BitSize  = BasepUnicodeStringToOemSize;
    Basep8BitStringToUnicodeSize = BasepOemStringToUnicodeSize;
}

VOID
WINAPI
SetFileApisToANSI(
    VOID
    )
{
    Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
    BasepUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
    BasepUnicodeStringTo8BitSize  = BasepUnicodeStringToAnsiSize;
    Basep8BitStringToUnicodeSize = BasepAnsiStringToUnicodeSize;
}

BOOL
WINAPI
AreFileApisANSI(
    VOID
    )
{
    return Basep8BitStringToUnicodeString == RtlAnsiStringToUnicodeString;
}    
BOOLEAN
ConDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );

BOOLEAN
NlsDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PBASE_STATIC_SERVER_DATA BaseStaticServerData
    );

BOOLEAN 
NlsThreadCleanup(void);


#if DBG
VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted (
    VOID
    );
#endif

#define REDIRECTION_EXTENSION L".Local"
    
BOOLEAN
BaseDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements Win32 base dll initialization.
    It's primary purpose is to create the Base heap.

Arguments:

    DllHandle - Saved in BaseDllHandle global variable

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    BOOLEAN Success;
    NTSTATUS Status;
    PPEB Peb;
    LPWSTR p, p1;
    BOOLEAN ServerProcess;
    HANDLE hNlsCacheMutant;
    USHORT Size;
#if !defined(BUILD_WOW6432)
    ULONG SizeMutant;
#endif
    WCHAR szSessionDir[MAX_SESSION_PATH];


    SessionId = NtCurrentPeb()->SessionId;

    BaseDllHandle = (HANDLE)DllHandle;
    
    (VOID)Context;

    Success = TRUE;

    Peb = NtCurrentPeb();

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:
 
        Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;

        RtlSetThreadPoolStartFunc( BaseCreateThreadPoolThread,
                                   BaseExitThreadPoolThread );

        BaseDllTag = RtlCreateTagHeap( RtlProcessHeap(),
                                       0,
                                       L"BASEDLL!",
                                       L"TMP\0"
                                       L"BACKUP\0"
                                       L"INI\0"
                                       L"FIND\0"
                                       L"GMEM\0"
                                       L"LMEM\0"
                                       L"ENV\0"
                                       L"RES\0"
                                       L"VDM\0"
                                     );

        BaseIniFileUpdateCount = 0;

        BaseDllInitializeMemoryManager();

        RtlInitUnicodeString( &BaseDefaultPath, NULL );

        //
        // Connect to BASESRV.DLL in the server process
        //

#if !defined(BUILD_WOW6432)
        SizeMutant = sizeof(hNlsCacheMutant);
#endif

        if ( SessionId == 0 ) {
           //
           // Console Session
           //
           wcscpy(szSessionDir, WINSS_OBJECT_DIRECTORY_NAME);
        } else {
           swprintf(szSessionDir,L"%ws\\%ld%ws",SESSION_ROOT,SessionId,WINSS_OBJECT_DIRECTORY_NAME);
        }

#if defined(BUILD_WOW6432) || defined(_WIN64)    
        Status = NtQuerySystemInformation(SystemBasicInformation,
                                          (PVOID)&SysInfo,
                                          sizeof(SYSTEM_BASIC_INFORMATION),
                                          NULL
                                         );

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

#endif

#if defined(BUILD_WOW6432)
        Status = CsrBaseClientConnectToServer(szSessionDir,
                                              &hNlsCacheMutant,
                                              &ServerProcess
                                             );
#else
        Status = CsrClientConnectToServer( szSessionDir,
                                           BASESRV_SERVERDLL_INDEX,
                                           NULL,
                                           &hNlsCacheMutant,
                                           &SizeMutant,
                                           &ServerProcess
                                         );
#endif

        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }

        BaseStaticServerData = BASE_SHARED_SERVER_DATA;

        if (!ServerProcess) {
            CsrNewThread();
            BaseRunningInServerProcess = FALSE;
            }
        else {
            BaseRunningInServerProcess = TRUE;
            }

        BaseCSDVersion = BaseStaticServerData->CSDVersion;
        BaseCSDNumber = BaseStaticServerData->CSDNumber;
        BaseRCNumber = BaseStaticServerData->RCNumber;
        if ((BaseCSDVersion) &&
            (!Peb->CSDVersion.Buffer)) {

            RtlInitUnicodeString(&Peb->CSDVersion, BaseCSDVersion);

        }

        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsDirectory, &BaseStaticServerData->WindowsDirectory);
        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsSystemDirectory, &BaseStaticServerData->WindowsSystemDirectory);

#ifdef WX86
        BASE_SERVER_STR_TO_LOCAL_STR(&BaseWindowsSys32x86Directory, &BaseStaticServerData->WindowsSys32x86Directory);
#endif

        RtlInitUnicodeString(&BaseConsoleInput,L"CONIN$");
        RtlInitUnicodeString(&BaseConsoleOutput,L"CONOUT$");
        RtlInitUnicodeString(&BaseConsoleGeneric,L"CON");

        BaseUnicodeCommandLine = *(PUNICODE_STRING)&(NtCurrentPeb()->ProcessParameters->CommandLine);
        Status = RtlUnicodeStringToAnsiString(
                    &BaseAnsiCommandLine,
                    &BaseUnicodeCommandLine,
                    TRUE
                    );
        if ( !NT_SUCCESS(Status) ){
            BaseAnsiCommandLine.Buffer = NULL;
            BaseAnsiCommandLine.Length = 0;
            BaseAnsiCommandLine.MaximumLength = 0;
            }

        //
        // Setup the global for this process that decides whether we want DLL
        // redirection on or not. LoadLibrary() and GetModulehandle() look at this
        // boolean.
        //
        // Note that BaseDefaultPathBuffer size is 2K (that is > MAX_PATH for sure)!
        //

        Size = (USHORT) GetModuleFileNameW(NULL, BaseDefaultPathBuffer, MAX_PATH);
        RtlCopyMemory(BaseDefaultPathBuffer+Size, REDIRECTION_EXTENSION, sizeof(REDIRECTION_EXTENSION));

        gDoDllRedirection = RtlDoesFileExists_U(BaseDefaultPathBuffer) ;

        p = BaseDefaultPathBuffer;
        *p++ = L'.';
        *p++ = L';';


        p1 = BaseWindowsSystemDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

#ifdef WX86

        //
        // Wx86 system dir follows 32 bit system dir
        //

        p1 = BaseWindowsSys32x86Directory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';
#endif


        //
        // 16bit system directory follows 32bit system directory
        //
        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        p1 = L"\\system";
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';

        p1 = BaseWindowsDirectory.Buffer;
        while( *p = *p1++) {
            p++;
            }
        *p++ = L';';
        *p = UNICODE_NULL;

        BaseDefaultPath.Buffer = BaseDefaultPathBuffer;
        BaseDefaultPath.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)BaseDefaultPathBuffer);
        BaseDefaultPath.MaximumLength = sizeof( BaseDefaultPathBuffer );

        BaseDefaultPathAppend.Buffer = p;
        BaseDefaultPathAppend.Length = 0;
        BaseDefaultPathAppend.MaximumLength = (USHORT)
            (BaseDefaultPath.MaximumLength - BaseDefaultPath.Length);

        RtlInitUnicodeString(&BasePathVariableName,L"PATH");
        RtlInitUnicodeString(&BaseUserProfileVariableName,L"USERPROFILE");
        RtlInitUnicodeString(&BaseTmpVariableName,L"TMP");
        RtlInitUnicodeString(&BaseTempVariableName,L"TEMP");
        RtlInitUnicodeString(&BaseDotVariableName,L".");
        RtlInitUnicodeString(&BaseDotTmpSuffixName,L".tmp");
        RtlInitUnicodeString(&BaseDotComSuffixName,L".com");
        RtlInitUnicodeString(&BaseDotPifSuffixName,L".pif");
        RtlInitUnicodeString(&BaseDotExeSuffixName,L".exe");

        BaseDllInitializeIniFileMappings( BaseStaticServerData );

    
        if ( Peb->ProcessParameters ) {
            if ( Peb->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {

                LoadLibrary("psapi.dll");

                }

            if (Peb->ProcessParameters->DebugFlags) {
                DbgBreakPoint();
                }
            }

        //
        // call the NLS API initialization routine
        //
        if ( !NlsDllInitialize( DllHandle,
                                Reason,
                                BaseStaticServerData ) )
        {
            return FALSE;
        }

        //
        // call the console initialization routine
        //
        if ( !ConDllInitialize(DllHandle,Reason,Context) ) {
            return FALSE;
            }


        //
        // Intialize TerminalServer(Hydra) hook function pointers for app compatibility
        //
        if (IsTerminalServer()) {


            InitializeTermsrvFpns();

        }
        
        InitializeListHead( &BasepAppCertDllsList );

        if (!NT_SUCCESS(RtlInitializeCriticalSection(&gcsAppCert))) {
           return FALSE;
        }

#if DBG

        AssertDelayLoadFailureMapsAreSorted ();
#endif

        break;

    case DLL_PROCESS_DETACH:

        //
        // Make sure any open registry keys are closed.
        //

        if (BaseIniFileUpdateCount != 0) {
            WriteProfileStringW( NULL, NULL, NULL );
            }

        break;

    case DLL_THREAD_ATTACH:
        //
        // call the console initialization routine
        //
        if ( !ConDllInitialize(DllHandle,Reason,Context) ) {
            return FALSE;
            }
        break;

    case DLL_THREAD_DETACH:

        //
        // Delete the thread NLS cache, if exists.
        //
        NlsThreadCleanup();

        break;

    default:
        break;
    }

    return Success;
}


HANDLE
BaseGetNamedObjectDirectory(
    VOID
    )
{
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    UNICODE_STRING RestrictedObjectDirectory;  // BUGBUG this should not be hardcoded
    ACCESS_MASK DirAccess = DIRECTORY_ALL_ACCESS &
                            ~(DELETE | WRITE_DAC | WRITE_OWNER);
    HANDLE hRootNamedObject;


    RtlAcquirePebLock();

    if ( !BaseNamedObjectDirectory ) {

        BASE_READ_REMOTE_STR_TEMP(TempStr);
        InitializeObjectAttributes( &Obja,
                                    BASE_READ_REMOTE_STR(BaseStaticServerData->NamedObjectDirectory, TempStr),
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );

        Status = NtOpenDirectoryObject( &BaseNamedObjectDirectory,
                                        DirAccess,
                                        &Obja
                                      );

        // if the intial open failed, try again with just traverse, and
        // open the restricted subdirectory

        if ( !NT_SUCCESS(Status) ) {
            Status = NtOpenDirectoryObject( &hRootNamedObject,
                                            DIRECTORY_TRAVERSE,
                                            &Obja
                                          );
            if ( NT_SUCCESS(Status) ) {
                RtlInitUnicodeString( &RestrictedObjectDirectory, L"Restricted");

                InitializeObjectAttributes( &Obja,
                                            &RestrictedObjectDirectory,
                                            OBJ_CASE_INSENSITIVE,
                                            hRootNamedObject,
                                            NULL
                                            );
                Status = NtOpenDirectoryObject( &BaseNamedObjectDirectory,
                                                DirAccess,
                                                &Obja
                                              );
                NtClose( hRootNamedObject );
                }

            if ( !NT_SUCCESS(Status) ) {
                BaseNamedObjectDirectory = NULL;
                }
            }
        }
    RtlReleasePebLock();
    return BaseNamedObjectDirectory;
}
