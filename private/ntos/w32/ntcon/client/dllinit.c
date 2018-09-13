/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module implements console dll initialization

Author:

    Therese Stowell (thereses) 11-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if !defined(BUILD_WOW64)

#include <cpl.h>

#define DEFAULT_WINDOW_TITLE (L"Command Prompt")

extern HANDLE InputWaitHandle;
extern WCHAR ExeNameBuffer[];
extern USHORT ExeNameLength;
extern WCHAR StartDirBuffer[];
extern USHORT StartDirLength;

DWORD
CtrlRoutine(
    IN LPVOID lpThreadParameter
    );

DWORD
PropRoutine(
    IN LPVOID lpThreadParameter
    );

#if defined(FE_SB)
#if defined(FE_IME)
DWORD
ConsoleIMERoutine(
    IN LPVOID lpThreadParameter
    );
#endif // FE_IME
#endif // FE_SB


#define MAX_SESSION_PATH   256
#define SESSION_ROOT       L"\\Sessions"

VOID
InitExeName( VOID );

BOOLEAN
ConsoleApp( VOID )

/*++

    This routine determines whether the current process is a console or
    windows app.

Parameters:

    none.

Return Value:

    TRUE if console app.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(GetModuleHandle(NULL));
    return (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) ? TRUE : FALSE;
}


VOID
SetUpAppName(
    IN OUT LPDWORD CurDirLength,
    OUT LPWSTR CurDir,
    IN OUT LPDWORD AppNameLength,
    OUT LPWSTR AppName
    )
{
    DWORD Length;

    *CurDirLength -= sizeof(WCHAR);
    Length = (StartDirLength*sizeof(WCHAR)) > *CurDirLength ? *CurDirLength : (StartDirLength*sizeof(WCHAR));
    RtlCopyMemory(CurDir,StartDirBuffer,Length+sizeof(WCHAR));
    *CurDirLength = Length + sizeof(WCHAR);   // add terminating NULL

    *AppNameLength -= sizeof(WCHAR);
    Length = (ExeNameLength*sizeof(WCHAR)) > *AppNameLength ? *AppNameLength : (ExeNameLength*sizeof(WCHAR));
    RtlCopyMemory(AppName,ExeNameBuffer,Length+sizeof(WCHAR));
    *AppNameLength = Length + sizeof(WCHAR);   // add terminating NULL
}


ULONG
ParseReserved(
    WCHAR *pchReserved,
    WCHAR *pchFind
    )
{
    ULONG dw;
    WCHAR *pch, *pchT, ch;
    UNICODE_STRING uString;

    dw = 0;
    if ((pch = wcsstr(pchReserved, pchFind)) != NULL) {
        pch += lstrlenW(pchFind);

        pchT = pch;
        while (*pchT >= '0' && *pchT <= '9')
            pchT++;

        ch = *pchT;
        *pchT = 0;
        RtlInitUnicodeString(&uString, pch);
        *pchT = ch;

        RtlUnicodeStringToInteger(&uString, 0, &dw);
    }

    return dw;
}


VOID
SetUpConsoleInfo(
    IN BOOL DllInit,
    OUT LPDWORD TitleLength,
    OUT LPWSTR Title OPTIONAL,
    OUT LPDWORD DesktopLength,
    OUT LPWSTR *Desktop OPTIONAL,
    OUT PCONSOLE_INFO ConsoleInfo
    )

/*++

    This routine fills in the ConsoleInfo structure with the values
    specified by the user.

Parameters:

    ConsoleInfo - pointer to structure to fill in.

Return Value:

    none.

--*/

{
    STARTUPINFOW StartupInfo;
    HANDLE h;
    int id;
    HANDLE ghInstance;
    BOOL Success;


    GetStartupInfoW(&StartupInfo);
    ghInstance = (HANDLE)((PVOID)NtCurrentPeb()->ImageBaseAddress );

    // these will eventually be filled in using menu input

    ConsoleInfo->nFont = 0;
    ConsoleInfo->nInputBufferSize = 0;
    ConsoleInfo->hIcon = NULL;
    ConsoleInfo->hSmIcon = NULL;
    ConsoleInfo->iIconId = 0;
    ConsoleInfo->dwStartupFlags = StartupInfo.dwFlags;
#if defined(FE_SB)
    ConsoleInfo->uCodePage = GetOEMCP();
#endif
    if (StartupInfo.lpTitle == NULL) {
        StartupInfo.lpTitle = DEFAULT_WINDOW_TITLE;
    }

    //
    // if the desktop name was specified, set up the pointers.
    //

    if (DllInit && Desktop != NULL &&
            StartupInfo.lpDesktop != NULL && *StartupInfo.lpDesktop != 0) {
        *DesktopLength = (lstrlenW(StartupInfo.lpDesktop) + 1) * sizeof(WCHAR);
        *Desktop = StartupInfo.lpDesktop;
    } else {
        *DesktopLength = 0;
        if (Desktop != NULL)
            *Desktop = NULL;
    }

    // Nope, do normal initialization (TitleLength is in BYTES, not CHARS!)
    *TitleLength = (USHORT)((lstrlenW(StartupInfo.lpTitle)+1)*sizeof(WCHAR));
    *TitleLength = (USHORT)(min(*TitleLength,MAX_TITLE_LENGTH));
    if (DllInit) {
        RtlCopyMemory(Title,StartupInfo.lpTitle,*TitleLength);
        // ensure the title is NULL terminated
        if (*TitleLength == MAX_TITLE_LENGTH)
            Title[ (MAX_TITLE_LENGTH/sizeof(WCHAR)) - 1 ] = L'\0';
    }

    if (StartupInfo.dwFlags & STARTF_USESHOWWINDOW) {
        ConsoleInfo->wShowWindow = StartupInfo.wShowWindow;
    }
    if (StartupInfo.dwFlags & STARTF_USEFILLATTRIBUTE) {
        ConsoleInfo->wFillAttribute = (WORD)StartupInfo.dwFillAttribute;
    }
    if (StartupInfo.dwFlags & STARTF_USECOUNTCHARS) {
        ConsoleInfo->dwScreenBufferSize.X = (WORD)(StartupInfo.dwXCountChars);
        ConsoleInfo->dwScreenBufferSize.Y = (WORD)(StartupInfo.dwYCountChars);
    }
    if (StartupInfo.dwFlags & STARTF_USESIZE) {
        ConsoleInfo->dwWindowSize.X = (WORD)(StartupInfo.dwXSize);
        ConsoleInfo->dwWindowSize.Y = (WORD)(StartupInfo.dwYSize);
    }
    if (StartupInfo.dwFlags & STARTF_USEPOSITION) {
        ConsoleInfo->dwWindowOrigin.X = (WORD)(StartupInfo.dwX);
        ConsoleInfo->dwWindowOrigin.Y = (WORD)(StartupInfo.dwY);
    }

    //
    // Grab information passed on lpReserved line...
    //

    if (StartupInfo.lpReserved != 0) {

        //
        // the program manager has an icon for the exe.  store the
        // index in the iIconId field.
        //

        ConsoleInfo->iIconId = ParseReserved(StartupInfo.lpReserved, L"dde.");

        //
        // The new "Chicago" way of doing things is to pass the hotkey in the
        // hStdInput field and set the STARTF_USEHOTKEY flag.  So, if this is
        // specified, we get the hotkey from there instead
        //

        if (StartupInfo.dwFlags & STARTF_USEHOTKEY) {
            ConsoleInfo->dwHotKey = HandleToUlong(StartupInfo.hStdInput);
        } else {
            ConsoleInfo->dwHotKey = ParseReserved(StartupInfo.lpReserved, L"hotkey.");
        }
    }

}

VOID
SetUpHandles(
    IN PCONSOLE_INFO ConsoleInfo
    )

/*++

    This routine sets up the console and std* handles for the process.

Parameters:

    ConsoleInfo - pointer to structure containing handles.

Return Value:

    none.

--*/

{
    SET_CONSOLE_HANDLE(ConsoleInfo->ConsoleHandle);
    if (!(ConsoleInfo->dwStartupFlags & STARTF_USESTDHANDLES)) {
        SetStdHandle(STD_INPUT_HANDLE,ConsoleInfo->StdIn);
        SetStdHandle(STD_OUTPUT_HANDLE,ConsoleInfo->StdOut);
        SetStdHandle(STD_ERROR_HANDLE,ConsoleInfo->StdErr);
    }
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
WINAPI
GetConsoleLangId(
    OUT LANGID *lpLangId
    )

/*++

Parameters:

    lpLangId - Supplies a pointer to a LANGID in which to store the Language ID.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_LANGID_MSG a = &m.u.GetConsoleLangId;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetLangId
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *lpLangId = a->LangId;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return FALSE;
        }
        return TRUE;
    } else {
        return FALSE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

VOID
SetTEBLangID(
    VOID
    )

/*++

    Sets the Language Id in the TEB to Far East if code page CP are
    Japanese/Korean/Chinese.  This is done in order for FormatMessage
    to display any Far East character when cmd is running in its code page.
    All messages displayed in non-FE code page will be displayed in English.

--*/

{
    LANGID LangId;

    if (GetConsoleLangId(&LangId)) {
        SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );
    }
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
ConnectConsoleInternal(IN PWSTR pObjectDirectory,
                       IN OUT PCONSOLE_API_CONNECTINFO pConnectInfo,
                       OUT PBOOLEAN pServerProcess
                      )
/*++

Routine Description:

    Helper function for establishing a connection with the console server.
    Waits for the server to signal completion.

Arguments:

    pObjectDirectory -  Supplies a null terminated string that is the same
        as the value of the ObjectDirectory= argument passed to the CSRSS
        program.

    pConnectInfo - Supplies and recieves the connection information.

    pServerProcess - Recieves TRUE if this is a server process.

Return Value:

    TRUE - Success

    FALSE - An error occured.

--*/
{

   NTSTATUS Status;
   ULONG ConnectionInformationLength;

   ConnectionInformationLength = sizeof(CONSOLE_API_CONNECTINFO);

   Status = CsrClientConnectToServer( pObjectDirectory,
                                      CONSRV_SERVERDLL_INDEX,
                                      NULL,
                                      pConnectInfo,
                                      &ConnectionInformationLength,
                                      pServerProcess
                                    );

   if (!NT_SUCCESS( Status )) {
       return FALSE;
   }

   //
   // we return success although no console api can be called because
   // loading shouldn't fail.  we'll fail the api calls later.
   //

   if (*pServerProcess) {
       return TRUE;
   }


   //
   // if this is not a console app, return success - nothing else to do.
   //

   if (!pConnectInfo->ConsoleApp) {
       return TRUE;
   }

   //
   // wait for initialization to complete.  we have to use the NT
   // wait because the heap hasn't been initialized yet.
   //

   Status = NtWaitForMultipleObjects(NUMBER_OF_INITIALIZATION_EVENTS,
                                        pConnectInfo->ConsoleInfo.InitEvents,
                                        WaitAny,
                                        FALSE,
                                        NULL
                                        );

   if (!NT_SUCCESS(Status)) {
       SET_LAST_NT_ERROR(Status);
       return FALSE;
   }

   NtClose(pConnectInfo->ConsoleInfo.InitEvents[INITIALIZATION_SUCCEEDED]);
   NtClose(pConnectInfo->ConsoleInfo.InitEvents[INITIALIZATION_FAILED]);
   if (Status != INITIALIZATION_SUCCEEDED) {
       SET_CONSOLE_HANDLE(NULL);
       return FALSE;
   }

   return TRUE;
}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOLEAN
ConDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements console dll initialization.

Arguments:

    DllHandle - Not Used

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    BOOL bStatus;
    CONSOLE_API_CONNECTINFO ConnectionInformation;
    BOOLEAN ServerProcess;

    ULONG SessionId = NtCurrentPeb()->SessionId;
    WCHAR szSessionDir[MAX_SESSION_PATH];

    Status = STATUS_SUCCESS;

    //
    // if we're attaching the DLL, we need to connect to the server.
    // if no console exists, we also need to create it and set up stdin,
    // stdout, and stderr.
    //

    if ( Reason == DLL_PROCESS_ATTACH ) {

        //
        // Remember in the connect information if this app is a console
        // app. need to actually connect to the console server for windowed
        // apps so that we know NOT to do any special work during
        // ConsoleClientDisconnectRoutine(). Store ConsoleApp info in the
        // CSR managed per-process data.
        //

        Status = RtlInitializeCriticalSection(&DllLock);
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        ConnectionInformation.CtrlRoutine = CtrlRoutine;
        ConnectionInformation.PropRoutine = PropRoutine;
#if defined(FE_SB)
#if defined(FE_IME)
        ConnectionInformation.ConsoleIMERoutine = ConsoleIMERoutine;
#endif // FE_IME
#endif // FE_SB

        ConnectionInformation.WindowVisible = TRUE;
        ConnectionInformation.ConsoleApp = ConsoleApp();
        if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_DETACHED_PROCESS) {
            SET_CONSOLE_HANDLE(NULL);
            ConnectionInformation.ConsoleApp = FALSE;
        }
        else if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_NEW_CONSOLE) {
            SET_CONSOLE_HANDLE(NULL);
        } else if (GET_CONSOLE_HANDLE == (HANDLE)CONSOLE_CREATE_NO_WINDOW) {
            SET_CONSOLE_HANDLE(NULL);
            ConnectionInformation.WindowVisible = FALSE;
        }
        if (!ConnectionInformation.ConsoleApp) {
            SET_CONSOLE_HANDLE(NULL);
        }
        ConnectionInformation.ConsoleInfo.ConsoleHandle = GET_CONSOLE_HANDLE;

        //
        // if no console exists, pass parameters for console creation
        //

        if (GET_CONSOLE_HANDLE == NULL && ConnectionInformation.ConsoleApp) {
            SetUpConsoleInfo(TRUE,
                             &ConnectionInformation.TitleLength,
                             ConnectionInformation.Title,
                             &ConnectionInformation.DesktopLength,
                             &ConnectionInformation.Desktop,
                             &ConnectionInformation.ConsoleInfo);
        } else {
            ConnectionInformation.TitleLength = 0;
            ConnectionInformation.DesktopLength = 0;
        }

        if (ConnectionInformation.ConsoleApp) {
            InitExeName();
            ConnectionInformation.CurDirLength = sizeof(ConnectionInformation.CurDir);
            ConnectionInformation.AppNameLength = sizeof(ConnectionInformation.AppName);
            SetUpAppName(&ConnectionInformation.CurDirLength,
                         ConnectionInformation.CurDir,
                         &ConnectionInformation.AppNameLength,
                         ConnectionInformation.AppName);
        } else {
            ConnectionInformation.AppNameLength = 0;
            ConnectionInformation.CurDirLength = 0;
        }

        //
        // initialize ctrl handling. This should work for all apps, so
        // initialize it before we check for ConsoleApp (which means the
        // console bit was set in the module header).
        //

        InitializeCtrlHandling();

        //
        // Connect to the server process
        //

        if ( SessionId == 0 ) {
           //
           // Console Session
           //
           wcscpy(szSessionDir, WINSS_OBJECT_DIRECTORY_NAME);
        } else {
           swprintf(szSessionDir,L"%ws\\%ld%ws",SESSION_ROOT,SessionId,WINSS_OBJECT_DIRECTORY_NAME);
        }

        bStatus = ConnectConsoleInternal(szSessionDir,
                                         &ConnectionInformation,
                                         &ServerProcess
                                         );

        if (!bStatus) {
           return FALSE;
        }

        //
        // we return success although no console api can be called because
        // loading shouldn't fail.  we'll fail the api calls later.
        //
        if (ServerProcess) {
           return TRUE;
        }

        //
        // if this is not a console app, return success - nothing else to do.
        //

        if (!ConnectionInformation.ConsoleApp) {
           return TRUE;
        }

        //
        // if console was just created, fill in peb values
        //

        if (GET_CONSOLE_HANDLE == NULL) {
            SetUpHandles(&ConnectionInformation.ConsoleInfo
                        );
        }

        InputWaitHandle = ConnectionInformation.ConsoleInfo.InputWaitHandle;

        SetTEBLangID();

    }
    else if ( Reason == DLL_THREAD_ATTACH ) {
        if (ConsoleApp()) {
            SetTEBLangID();
        }
    }
    return TRUE;
    UNREFERENCED_PARAMETER(DllHandle);
    UNREFERENCED_PARAMETER(Context);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
AllocConsoleInternal(IN LPWSTR lpTitle,
                     IN DWORD dwTitleLength,
                     IN LPWSTR lpDesktop,
                     IN DWORD dwDesktopLength,
                     IN LPWSTR lpCurDir,
                     IN DWORD dwCurDirLength,
                     IN LPWSTR lpAppName,
                     IN DWORD dwAppNameLength,
                     IN LPTHREAD_START_ROUTINE CtrlRoutine,
                     IN LPTHREAD_START_ROUTINE PropRoutine,
                     IN OUT PCONSOLE_INFO pConsoleInfo
                     )
/*++

Routine Description:

   Marshels the parameters for the ConsolepAlloc command.

Arguments:

   See the CONSOLE_ALLOC_MSG structure and AllocConsole.

Return Value:

    TRUE - Success

    FALSE - An error occured.

--*/
{
   CONSOLE_API_MSG m;
   PCONSOLE_ALLOC_MSG a = &m.u.AllocConsole;
   PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
   BOOL Status = FALSE;
   NTSTATUS St;

   try {

        a->CtrlRoutine = CtrlRoutine;
        a->PropRoutine = PropRoutine;

        // Allocate 4 extra pointer sizes to compensate for any alignment done
        // by CsrCaptureMessageBuffer.

        CaptureBuffer = CsrAllocateCaptureBuffer( 5,
                                                  dwTitleLength + dwDesktopLength + dwCurDirLength +
                                                  dwAppNameLength + sizeof( CONSOLE_INFO ) + (4 * sizeof(PVOID))
                                                 );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            Status = FALSE;
            leave;
        }

        // Allocate the CONSOLE_INFO first so that it is aligned on a pointer
        // boundry.  This is necessary since NtWaitForMultipleObject expects
        // its arguments aligned on a handle boundry.

        CsrCaptureMessageBuffer( CaptureBuffer,
                                 pConsoleInfo,
                                 sizeof( CONSOLE_INFO ),
                                 (PVOID *) &a->ConsoleInfo
                               );

        a->TitleLength = dwTitleLength;
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpTitle,
                                 dwTitleLength,
                                 (PVOID *) &a->Title
                               );

        a->DesktopLength = dwDesktopLength;
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpDesktop,
                                 dwDesktopLength,
                                 (PVOID *) &a->Desktop
                               );

        a->CurDirLength = dwCurDirLength;
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpCurDir,
                                 dwCurDirLength,
                                 (PVOID *) &a->CurDir
                               );

        a->AppNameLength = dwAppNameLength;
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpAppName,
                                 dwAppNameLength,
                                 (PVOID *) &a->AppName
                               );

        //
        // Connect to the server process
        //

        CsrClientCallServer( (PCSR_API_MSG)&m,
                             CaptureBuffer,
                             CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                                  ConsolepAlloc
                                                ),
                             sizeof( *a )
                           );
        if (!NT_SUCCESS( m.ReturnValue )) {
            SET_LAST_NT_ERROR (m.ReturnValue);
            Status = FALSE;
            leave;
        }

        St = NtWaitForMultipleObjects(NUMBER_OF_INITIALIZATION_EVENTS,
                                      a->ConsoleInfo->InitEvents,
                                      WaitAny,
                                      FALSE,
                                      NULL
                                      );
        if (!NT_SUCCESS(St)) {
           SET_LAST_NT_ERROR(St);
           Status = FALSE;
           leave;
        }

        //The handles to be closed are events, so NtClose works fine.
        NtClose(a->ConsoleInfo->InitEvents[INITIALIZATION_SUCCEEDED]);
        NtClose(a->ConsoleInfo->InitEvents[INITIALIZATION_FAILED]);
        if (St != INITIALIZATION_SUCCEEDED) {
            SET_CONSOLE_HANDLE(NULL);
            Status = FALSE;
            leave;
        }
        RtlCopyMemory(pConsoleInfo, a->ConsoleInfo, sizeof(CONSOLE_INFO));
        Status = TRUE;
   }
   finally {
      if (CaptureBuffer) {
         CsrFreeCaptureBuffer( CaptureBuffer );
      }
   }

   return Status;
}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
AllocConsole( VOID )

/*++

Routine Description:

    This API creates a console for the calling process.

Arguments:

    none.

Return Value:

    TRUE - function was successful.

--*/

{
    CONSOLE_INFO ConsoleInfo;
    STARTUPINFOW StartupInfo;
    WCHAR CurDir[MAX_PATH+1];
    WCHAR AppName[MAX_APP_NAME_LENGTH/2];
    BOOL Status = FALSE;

    DWORD dwTitleLength;
    DWORD dwDesktopLength;
    DWORD dwCurDirLength;
    DWORD dwAppNameLength;

    LockDll();
    try {
        if (GET_CONSOLE_HANDLE != NULL) {
            SetLastError(ERROR_ACCESS_DENIED);
            Status = FALSE;
            leave;
        }

        //
        // set up initialization parameters
        //

        SetUpConsoleInfo(FALSE,
                         &dwTitleLength,
                         NULL,
                         &dwDesktopLength,
                         NULL,
                         &ConsoleInfo);

        InitExeName();
        dwCurDirLength = sizeof(CurDir);
        dwAppNameLength = sizeof(AppName);
        SetUpAppName(&dwCurDirLength,
                     CurDir,
                     &dwAppNameLength,
                     AppName);

        GetStartupInfoW(&StartupInfo);

        if (StartupInfo.lpTitle == NULL) {
            StartupInfo.lpTitle = DEFAULT_WINDOW_TITLE;
        }
        dwTitleLength = (USHORT)((lstrlenW(StartupInfo.lpTitle)+1)*sizeof(WCHAR));
        dwTitleLength = (USHORT)(min(dwTitleLength,MAX_TITLE_LENGTH));
        if (StartupInfo.lpDesktop != NULL && *StartupInfo.lpDesktop != 0) {
            dwDesktopLength = (USHORT)((lstrlenW(StartupInfo.lpDesktop)+1)*sizeof(WCHAR));
            dwDesktopLength = (USHORT)(min(dwDesktopLength,MAX_TITLE_LENGTH));
        } else {
            dwDesktopLength = 0;
        }

        Status = AllocConsoleInternal(StartupInfo.lpTitle,
                                      dwTitleLength,
                                      StartupInfo.lpDesktop,
                                      dwDesktopLength,
                                      CurDir,
                                      dwCurDirLength,
                                      AppName,
                                      dwAppNameLength,
                                      CtrlRoutine,
                                      PropRoutine,
                                      &ConsoleInfo
                                      );

        if (!Status) {
           leave;
        }

        //
        // fill in peb values
        //

        SetUpHandles(&ConsoleInfo);

        //
        // create ctrl-c thread
        //

        InitializeCtrlHandling();

        InputWaitHandle = ConsoleInfo.InputWaitHandle;

        SetTEBLangID();

        Status = TRUE;

    } finally {
        UnlockDll();
    }

    return Status;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
FreeConsoleInternal(
     VOID
     )
/*++

Routine Description:

   Marshels the parameters for the ConsolepFree command.

Arguments:

   See the CONSOLE_FREE_MSG structure and FreeConsole.

Return Value:

    TRUE - Success

    FALSE - An error occured.

--*/
{

   CONSOLE_API_MSG m;
   PCONSOLE_FREE_MSG a = &m.u.FreeConsole;

   a->ConsoleHandle = GET_CONSOLE_HANDLE;

   //
   // Connect to the server process
   //

   CsrClientCallServer( (PCSR_API_MSG)&m,
                        NULL,
                        CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                             ConsolepFree
                                           ),
                        sizeof( *a )
                      );

   if (!NT_SUCCESS( m.ReturnValue )) {
      SET_LAST_NT_ERROR (m.ReturnValue);
      return FALSE;

   } else {

      SET_CONSOLE_HANDLE(NULL);
      return TRUE;
   }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
FreeConsole( VOID )

/*++

Routine Description:

    This API frees the calling process's console.

Arguments:

    none.

Return Value:

    TRUE - function was successful.

--*/

{
    BOOL Success=TRUE;

    LockDll();
    if (GET_CONSOLE_HANDLE == NULL) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        Success = FALSE;
    } else {

        Success = FreeConsoleInternal();

        if (Success) {
           CloseHandle(InputWaitHandle);
        }

    }
    UnlockDll();
    return Success;
}


DWORD
PropRoutine(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This thread is created when the user tries to change console
    properties from the system menu. It invokes the control panel
    applet.

Arguments:

    lpThreadParameter - not used.

Return Value:

    STATUS_SUCCESS - function was successful

--*/

{
    NTSTATUS Status;
    HANDLE hLibrary;
    APPLET_PROC pfnCplApplet;
    static BOOL fInPropRoutine = FALSE;

    //
    // Prevent the user from launching multiple applets attached
    // to a single console
    //

    if (fInPropRoutine) {
        if (lpThreadParameter) {
            CloseHandle((HANDLE)lpThreadParameter);
        }
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

    fInPropRoutine = TRUE;
    hLibrary = LoadLibraryW(L"CONSOLE.DLL");
    if (hLibrary != NULL) {
        pfnCplApplet = (APPLET_PROC)GetProcAddress(hLibrary, "CPlApplet");
        if (pfnCplApplet != NULL) {
            (*pfnCplApplet)((HWND)lpThreadParameter, CPL_INIT, 0, 0);
            (*pfnCplApplet)((HWND)lpThreadParameter, CPL_DBLCLK, 0, 0);
            (*pfnCplApplet)((HWND)lpThreadParameter, CPL_EXIT, 0, 0);
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }
        FreeLibrary(hLibrary);
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }
    fInPropRoutine = FALSE;

    return Status;
}

#endif //!defined(BUILD_WOW64)
