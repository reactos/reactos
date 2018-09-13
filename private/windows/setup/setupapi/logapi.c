/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    logapi.c

Abstract:

    Public exposure of an error logging API, based on windows\setup\setuplog.

Author:

    Jim Schmidt (jimschm) 28-Apr-1997

Revision History:

    jimschm     16-Dec-1998     Added UseCountCs (duh!!)

--*/

#include "precomp.h"

#include <setuplog.h>

SETUPLOG_CONTEXT LogContext;
INT UseCount;
CRITICAL_SECTION UseCountCs;

#define MAX_STRING_RESOURCE   0x08000



//
// NOTE: Watch the case.  We expose an API named SetupLogError, which is different than
//       the lib-based SetuplogError function.
//


VOID
InitLogApi (
    VOID
    )
{
    InitializeCriticalSection (&UseCountCs);
}


VOID
TerminateLogApi (
    VOID
    )
{
    DeleteCriticalSection (&UseCountCs);
}


LPSTR
pUnicodeToAnsiForDisplay (
    PCWSTR UnicodeStr
    )
{
    INT Len;
    LPSTR AnsiBuffer;
    CHAR CodePage[32];
    DWORD rc;

    //
    // Allocate buffer to be freed by caller
    //

    Len = (lstrlenW (UnicodeStr) + 1) * sizeof (WCHAR);

    AnsiBuffer = (LPSTR) MyMalloc (Len);
    if (!AnsiBuffer) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    //
    // Convert to UNICODE based on thread's Locale; convert assuming string
    // is for display purposes
    //

    if (!GetLocaleInfoA (GetThreadLocale(), LOCALE_IDEFAULTCODEPAGE, CodePage, 32)) {
        MyFree (AnsiBuffer);
        return NULL;
    }

    rc = WideCharToMultiByte (
            atoi (CodePage),
            WC_COMPOSITECHECK|WC_DISCARDNS,
            UnicodeStr,
            -1,
            AnsiBuffer,
            Len,
            NULL,
            NULL
            );

    if (rc == 0) {
        MyFree (AnsiBuffer);
        return NULL;
    }

    return AnsiBuffer;
}


PWSTR
pAnsiToUnicodeForDisplay (
    LPCSTR AnsiStr
    )
{
    INT Len;
    LPWSTR UnicodeBuffer;
    CHAR CodePage[32];
    DWORD rc;

    //
    // Allocate buffer to be freed by caller
    //

    Len = (lstrlenA (AnsiStr) + 1) * sizeof (WCHAR);

    UnicodeBuffer = (LPWSTR) MyMalloc (Len);
    if (!UnicodeBuffer) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    //
    // Convert to UNICODE based on thread's Locale
    //

    if (!GetLocaleInfoA (GetThreadLocale(), LOCALE_IDEFAULTCODEPAGE, CodePage, 32)) {
        MyFree (UnicodeBuffer);
        return NULL;
    }

    rc = MultiByteToWideChar (
            atoi (CodePage),
            MB_USEGLYPHCHARS,
            AnsiStr,
            -1,
            UnicodeBuffer,
            Len
            );

    if (rc == 0) {
        MyFree (UnicodeBuffer);
        return NULL;
    }

    return UnicodeBuffer;
}


PVOID
pOpenFileCallback (
    IN  LPCTSTR  Filename,
    IN  BOOL     WipeLogFile
    )

/*++

Routine Description:

    Opens the log and optionally overwrites an existing copy.

Arguments:

    FileName    - Specifies the name of the file to open or create

    WipeLogFile - TRUE if an existing log should be overwritten, FALSE if
                  it should be appended

Return Value:

    Pointer to the file handle.

--*/


{
    TCHAR   CompleteFilename[MAX_PATH*2];
    HANDLE  hFile;

    //
    // Form the pathname of the logfile.
    //
    // uses real windows directory BUGBUG!!! (jamiehun) make sure this is correct for Hydra
    //
    lstrcpy(CompleteFilename,WindowsDirectory);
    if (!ConcatenatePaths (CompleteFilename, Filename, MAX_PATH, NULL)) {
        return NULL;
    }

    //
    // If we're wiping the logfile clean, attempt to delete
    // what's there.
    //
    if(WipeLogFile) {
        SetFileAttributes (CompleteFilename, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (CompleteFilename);
    }

    //
    // Open existing file or create a new one.
    //
    hFile = CreateFile (
        CompleteFilename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    return (PVOID)hFile;
}


static
BOOL
pWriteFile (
    IN  PVOID   LogFile,
    IN  LPCTSTR Buffer
    )

/*++

Routine Description:

    Writes an entry to the Setup Error Log by converting string to ANSI and
    calling WriteFile.  The message is appended to the log.

Arguments:

    LogFile  - The handle to an open log file
    Buffer   - The UNICODE message to write

Return Value:

    Boolean indicating whether the operation was successful.  Error code is set
    to a Win32 error code if the return value is FALSE.

--*/


{
    PCSTR   AnsiBuffer;
    BOOL    Status;
    DWORD   DontCare;

    if (0xffffffff == SetFilePointer (LogFile, 0, NULL, FILE_END)) {
        return FALSE;
    }

#ifdef UNICODE

    //
    // Convert to ANSI for file output
    //

    if (AnsiBuffer = pUnicodeToAnsiForDisplay (Buffer)) {
        Status = WriteFile (
                    LogFile,
                    AnsiBuffer,
                    lstrlenA (AnsiBuffer),
                    &DontCare,
                    NULL
                    );
        MyFree (AnsiBuffer);
    } else {
        Status = FALSE;
    }

#else

    Status = WriteFile (
                LogFile,
                Buffer,
                lstrlen (Buffer),
                &DontCare,
                NULL
                );

#endif

    if (Status) {
        FlushFileBuffers (LogFile);
    }

    return Status;

}


static
LPTSTR
pFormatLogMessage (
    IN LPCTSTR   MessageString,
    IN UINT      MessageId,      OPTIONAL
    IN va_list * ArgumentList
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    This routine supports only MessageIds that are Win32 error codes.  It
    does not support messages for string resources.

Arguments:

    MessageString - Supplies the message text.  For logapi.c, this should
                    always be non-NULL.

    MessageId - Supplies a Win32 error code, or 0 if MessageString is to be
                used.

    ArgumentList - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    DWORD d;
    LPTSTR Buffer;
    LPTSTR Message;
    TCHAR  ModuleName[MAX_PATH];
    TCHAR  ErrorNumber[24];
    LPTSTR Args[2];

    if (MessageString > (LPCTSTR) SETUPLOG_USE_MESSAGEID) {
        d = FormatMessage (
                FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                MessageString,
                0,
                0,
                (LPTSTR) &Buffer,
                0,
                ArgumentList
                );
    } else {
        d = FormatMessage (
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    ((MessageId < MSG_FIRST) ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE),
                (PVOID) GetModuleHandle (NULL),
                MessageId,
                MAKELANGID (LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (LPTSTR) &Buffer,
                0,
                ArgumentList
                );
    }


    if(!d) {
        //
        // Give up.
        //
        return NULL;
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Message = DuplicateString (Buffer);

    LocalFree ((HLOCAL) Buffer);

    return Message;
}


static
BOOL
pAcquireMutex (
    IN  PVOID   Mutex
    )

/*++

Routine Description:

    Waits on the log mutex for a max of 1 second, and returns TRUE if the mutex
    was claimed, or FALSE if the claim timed out.

Arguments:

    Mutex - specifies which mutex to acquire.

Return Value:

    TRUE if the mutex was claimed, or FALSE if the claim timed out.

--*/


{
    DWORD rc;

    if (!Mutex) {
        SetLastError (ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Wait a max of 1 second for the mutex
    rc = WaitForSingleObject (Mutex, 1000);
    if (rc != WAIT_OBJECT_0) {
        SetLastError (ERROR_EXCL_SEM_ALREADY_OWNED);
        return FALSE;
    }

    return TRUE;
}



BOOL
WINAPI
SetupOpenLog (
    BOOL Erase
    )

/*++

Routine Description:

    Opens the log for processing.  Must be called before SetupLogError is called.
    A use count is maintained so a single process can call SetupOpenLog and
    SetupCloseLog from multiple threads.

Arguments:

    Erase - TRUE to erase an existing log, or FALSE to append to an existing log

Return Value:

    Boolean indicating whether the operation was successful.  Error code is set
    to a Win32 error code if the return value is FALSE.

--*/

{
    BOOL b = TRUE;
    INT i;
    DWORD rc;

    EnterCriticalSection (&UseCountCs);

    __try {
        //
        // Perform initialization of log APIs
        //

        if (!UseCount) {
            LogContext.OpenFile  = (PSPLOG_OPENFILE_ROUTINE) pOpenFileCallback;
            LogContext.CloseFile = CloseHandle;
            LogContext.AllocMem  = MyMalloc;
            LogContext.FreeMem   = MyFree;
            LogContext.Format    = (PSPLOG_FORMAT_ROUTINE) pFormatLogMessage;
            LogContext.Write     = (PSPLOG_WRITE_ROUTINE) pWriteFile;
            LogContext.Lock      = pAcquireMutex;
            LogContext.Unlock    = ReleaseMutex;

            LogContext.Mutex = CreateMutexW(NULL,FALSE,L"SetuplogMutex");

            for (i = 0 ; i < LogSevMaximum ; i++) {
                LogContext.SeverityDescriptions[i] = MyLoadString (IDS_LOGSEVINFORMATION + i);
            }

            //
            // We don't want to allow anyone to erase the existing log, so we just
            // ignore the value of Erase and always append to the log.
            //
            b = SetuplogInitialize (&LogContext, FALSE);
            rc = GetLastError();

        } else {
            rc = ERROR_ALREADY_INITIALIZED;
        }

        UseCount++;
    }
    __finally {
        //
        // Clean up and exit
        //

        if (!b) {
            SetupCloseLog();
        }

        SetLastError (rc);
        LeaveCriticalSection (&UseCountCs);
    }

    return b;
}


VOID
WINAPI
SetupCloseLog (
    VOID
    )

/*++

Routine Description:

    Cleans up all resources associated with the log

Arguments:

    none

Return Value:

    none

--*/


{
    INT i;

    EnterCriticalSection (&UseCountCs);

    __try {
        if (!UseCount) {
            __leave;
        }

        UseCount--;
        if (!UseCount) {
            if(LogContext.Mutex) {
                CloseHandle(LogContext.Mutex);
                LogContext.Mutex = NULL;
            }

            for (i=0; i<LogSevMaximum; i++) {
                if (LogContext.SeverityDescriptions[i]) {
                    MyFree (LogContext.SeverityDescriptions[i]);
                }
            }

            SetuplogTerminate();
        }
    }
    __finally {
        LeaveCriticalSection (&UseCountCs);
    }
}


BOOL
WINAPI
SetupLogErrorA (
    IN  PCSTR               MessageString,
    IN  LogSeverity         Severity
    )

/*++

Routine Description:

    Writes an entry to the Setup Error Log.  If we're being compiled UNICODE,
    we convert the MessageString to UNICODE and call SetupLogErrorW.  If we're
    being compiled ANSI, we call the log API directly.

Arguments:

    MessageString       - Pointer to a buffer containing unformatted message text

    Severity            - Severity of the error:

                          LogSevInformation
                          LogSevWarning
                          LogSevError
                          LogSevFatalError

Return Value:

    Boolean indicating whether the operation was successful.  Error code is set
    to a Win32 error code if the return value is FALSE.

--*/

{
    INT Len;
    PWSTR UnicodeBuffer;
    BOOL b = FALSE;
    CHAR CodePage[32];
    DWORD rc;

    __try {

        if (!UseCount) {
            rc = ERROR_FILE_INVALID;
        } else {

#ifdef UNICODE
            UnicodeBuffer = pAnsiToUnicodeForDisplay (MessageString);

            //
            // Call UNICODE version of the log API, preserve error code
            //

            if (UnicodeBuffer) {
                b = SetupLogErrorW (UnicodeBuffer, Severity);
                rc = GetLastError();
                MyFree (UnicodeBuffer);
            } else {
                rc = GetLastError();
            }

#else
            //
            // ANSI version -- call SetuplogError directly
            //

            b = SetuplogError (Severity, "%1", 0, MessageString, 0, 0);
            rc = GetLastError();

#endif
        }
    }

    __except (TRUE) {
        //
        // If caller passes in bogus pointer, fail with invalid parameter error
        //

        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
    }

    SetLastError(rc);
    return b;
}



BOOL
WINAPI
SetupLogErrorW (
    IN  PCWSTR              MessageString,
    IN  LogSeverity         Severity
    )

/*++

Routine Description:

    Writes an entry to the Setup Error Log.  If compiled with UNICODE, we call the
    SetuplogError function directly.  If compiled with ANSI, we convert to ANSI
    and call SetupLogErrorA.

Arguments:

    MessageString       - Pointer to a buffer containing unformatted message text

    Severity            - Severity of the error:

                          LogSevInformation
                          LogSevWarning
                          LogSevError
                          LogSevFatalError

Return Value:

    Boolean indicating whether the operation was successful.  Error code is set
    to a Win32 error code if the return value is FALSE.

--*/

{
    BOOL b = FALSE;
    PCSTR AnsiBuffer;
    DWORD rc;

    __try {

        if (!UseCount) {
            rc = ERROR_FILE_INVALID;
        } else {

#ifdef UNICODE
            //
            // UNICODE version: Call SetuplogError directly
            //

            // Log the error -- we always link to a UNICODE SetuplogError, despite the TCHAR header file
            b = SetuplogError (Severity, L"%1", 0, MessageString, 0, 0);
            rc = GetLastError();

#else
            //
            // ANSI version: Convert down to ANSI, then call SetupLogErrorA
            //

            AnsiBuffer = pUnicodeToAnsiForDisplay (MessageString);

            if (AnsiBuffer) {
                b = SetupLogErrorA (AnsiBuffer, Severity);
                rc = GetLastError();
                MyFree (AnsiBuffer);
            } else {
                rc = GetLastError();
            }

#endif
        }
    }
    __except (TRUE) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
    }

    SetLastError(rc);
    return b;
}


