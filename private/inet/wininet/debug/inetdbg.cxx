/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inetdbg.cxx

Abstract:

    Debugging functions for internet DLL

    Contents:
        InternetDebugInitialize
        InternetDebugTerminate
        InternetGetDebugInfo
        InternetSetDebugInfo
        InternetOpenDebugFile
        InternetReopenDebugFile
        InternetCloseDebugFile
        InternetFlushDebugFile
        InternetDebugSetControlFlags
        InternetDebugResetControlFlags
        InternetDebugEnter
        InternetDebugLeave
        InternetDebugError
        InternetDebugPrint
        (InternetDebugPrintString)
        InternetDebugPrintf
        InternetDebugOut
        InternetDebugDump
        InternetDebugDumpFormat
        InternetAssert
        InternetGetDebugVariable
        (InternetGetDebugVariableString)
        InternetMapError
        InternetMapStatus
        InternetMapOption
        InternetMapHttpOption
        InternetMapHttpState
        InternetMapHttpStateFlag
        InternetMapAsyncRequest
        InternetMapHandleType
        InternetMapScheme
        InternetMapOpenType
        InternetMapService
        (ExtractFileName)
        (SetDebugPrefix)
        SourceFilename
        InitSymLib
        TermSymLib
        GetDebugSymbol
        x86SleazeCallStack
        x86SleazeCallersAddress

Author:

    Richard L Firth (rfirth) 13-Feb-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    13-Feb-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include <ieverp.h>
#include "rprintf.h"
#include <imagehlp.h>
#include "autodial.h"

#ifdef ENABLE_DEBUG

//
// private manifests
//

#define SWITCH_VARIABLE_NAME        "WininetDebugging"
#define CONTROL_VARIABLE_NAME       "WininetControl"
#define CATEGORY_VARIABLE_NAME      "WininetCategory"
#define ERROR_VARIABLE_NAME         "WininetError"
#define BREAK_VARIABLE_NAME         "WininetBreak"
#define DEFAULT_LOG_VARIABLE_NAME   "WininetLog"
#define CHECK_LIST_VARIABLE_NAME    "WininetCheckSerializedList"
#define LOG_FILE_VARIABLE_NAME      "WininetLogFile"
#define INDENT_VARIABLE_NAME        "WininetLogIndent"

#define DEFAULT_LOG_FILE_NAME       "WININET.LOG"

#define ENVIRONMENT_VARIABLE_BUFFER_LENGTH  80

#define PRINTF_STACK_BUFFER_LENGTH  (4 K)

//
// private macros
//

#define CASE_OF(constant)   case constant: return # constant

//
// private prototypes
//

PRIVATE
VOID
InternetDebugPrintString(
    IN LPSTR String
    );

PRIVATE
VOID
InternetGetDebugVariableString(
    IN LPSTR lpszVariableName,
    OUT LPSTR lpszVariable,
    IN DWORD dwVariableLen
    );

PRIVATE
LPSTR
ExtractFileName(
    IN LPSTR Module,
    OUT LPSTR Buf
    );

PRIVATE
LPSTR
SetDebugPrefix(
    IN LPSTR Buffer
    );
//
//
// these variables are employed in macros, so must be public
//

PUBLIC DWORD InternetDebugErrorLevel = DBG_ERROR;
PUBLIC DWORD InternetDebugControlFlags = DBG_NO_DEBUG;
PUBLIC DWORD InternetDebugCategoryFlags = 0;
PUBLIC DWORD InternetDebugBreakFlags = 0;

//
// these variables are only accessed in this module, so can be private
//

PRIVATE int InternetDebugIndentIncrement = 2;
PRIVATE HANDLE InternetDebugFileHandle = INVALID_HANDLE_VALUE;
PRIVATE char InternetDebugFilename[MAX_PATH + 1] = DEFAULT_LOG_FILE_NAME;
PRIVATE BOOL InternetDebugEnabled = TRUE;
PRIVATE DWORD InternetDebugStartTime = 0;

extern "C" {
BOOL UrlcacheDebugEnabled = FALSE;
#if defined(UNIX) && defined(ux10)
/* Temporary fix for Apogee Compiler bug on HP only */
extern BOOL fCheckEntryOnList;
#else
BOOL fCheckEntryOnList;
#endif /* UNIX */
}

//
// high frequency performance counter globals
//


PRIVATE LONGLONG ftInit;  // initial local time
PRIVATE LONGLONG pcInit;  // initial perf counter
PRIVATE LONGLONG pcFreq;  // perf counter frequency

//
// functions
//


VOID
InternetDebugInitialize(
    VOID
    )

/*++

Routine Description:

    reads environment INETDBG flags and opens debug log file if required

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // ensure registry key open (normally done in GlobalDataInitialize() later)
    //

    OpenInternetSettingsKey();

    //
    // record the starting tick count for cumulative deltas
    //

    InternetDebugStartTime = GetTickCount();

    if (QueryPerformanceFrequency ((LARGE_INTEGER *) &pcFreq) && pcFreq) {

        QueryPerformanceCounter ((LARGE_INTEGER *) &pcInit);
        SYSTEMTIME st;
        GetLocalTime (&st);
        SystemTimeToFileTime (&st, (FILETIME *) &ftInit);
    }

    //
    // check see if there are any debug variable overrides in the environment
    // or the registry. If "WininetLog=<!0>" is set then we use the flags that
    // are most commonly used to generate WININET.LOG, with no console or
    // debugger output. We allow the other variables to be overridden
    //

    BOOL defaultDebugVariables = FALSE;

    InternetGetDebugVariable(DEFAULT_LOG_VARIABLE_NAME, (LPDWORD)&defaultDebugVariables);
    if (defaultDebugVariables) {
        InternetDebugEnabled = TRUE;
        InternetDebugControlFlags = INTERNET_DEBUG_CONTROL_DEFAULT;
        InternetDebugCategoryFlags = INTERNET_DEBUG_CATEGORY_DEFAULT;
        InternetDebugErrorLevel = INTERNET_DEBUG_ERROR_LEVEL_DEFAULT;
        InternetDebugBreakFlags = 0;
    }
    InternetGetDebugVariable(SWITCH_VARIABLE_NAME, (LPDWORD)&InternetDebugEnabled);
    InternetGetDebugVariable(CONTROL_VARIABLE_NAME, &InternetDebugControlFlags);
    InternetGetDebugVariable(CATEGORY_VARIABLE_NAME, &InternetDebugCategoryFlags);
    InternetGetDebugVariable(ERROR_VARIABLE_NAME, &InternetDebugErrorLevel);
    InternetGetDebugVariable(BREAK_VARIABLE_NAME, &InternetDebugBreakFlags);
    InternetGetDebugVariable(CHECK_LIST_VARIABLE_NAME, (LPDWORD)&fCheckEntryOnList);
    InternetGetDebugVariable(INDENT_VARIABLE_NAME, (LPDWORD)&InternetDebugIndentIncrement);
    InternetGetDebugVariableString(LOG_FILE_VARIABLE_NAME,
                                   InternetDebugFilename,
                                   sizeof(InternetDebugFilename)
                                   );

    UrlcacheDebugEnabled = InternetDebugEnabled &&
        (InternetDebugCategoryFlags & DBG_CACHE);

    if ((InternetDebugIndentIncrement < 0) || (InternetDebugIndentIncrement > 32)) {
        InternetDebugIndentIncrement = 2;
    }

    //
    // quit now if debugging is disabled
    //

    if (!InternetDebugEnabled) {
        InternetDebugControlFlags |= (DBG_NO_DEBUG | DBG_NO_DATA_DUMP);
        return;
    }

    //
    // if we want to write debug output to file, open WININET.LOG in the current
    // directory. Open it in text mode, for write-only (by this process)
    //

    if (InternetDebugControlFlags & DBG_TO_FILE) {
        if (!InternetReopenDebugFile(InternetDebugFilename)) {
            InternetDebugControlFlags &= ~DBG_TO_FILE;
        }
    }

    //
    // install the debug exception handler
    //

    SetExceptionHandler();
}


VOID
InternetDebugTerminate(
    VOID
    )

/*++

Routine Description:

    Performs any required debug termination

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_TO_FILE) {
        InternetCloseDebugFile();
    }
    InternetDebugControlFlags = DBG_NO_DEBUG;
}

DWORD
InternetGetDebugInfo(
    OUT LPINTERNET_DEBUG_INFO lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the internal debug variables

Arguments:

    lpBuffer            - pointer to structure that receives the variables

    lpdwBufferLength    - IN: Length of buffer
                          OUT: length of returned data if successful, else
                          required length of buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS;

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD requiredLength;
    DWORD error;
    int filenameLength;

    filenameLength = ((InternetDebugFileHandle != INVALID_HANDLE_VALUE)
                        ? strlen(InternetDebugFilename) : 0) + 1;

    requiredLength = sizeof(*lpBuffer) + filenameLength;
    if ((lpBuffer != NULL) && (*lpdwBufferLength >= requiredLength)) {
        lpBuffer->ErrorLevel = InternetDebugErrorLevel;
        lpBuffer->ControlFlags = InternetDebugControlFlags;
        lpBuffer->CategoryFlags = InternetDebugCategoryFlags;
        lpBuffer->BreakFlags = InternetDebugBreakFlags;
        lpBuffer->IndentIncrement = InternetDebugIndentIncrement;
        if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
            memcpy(lpBuffer->Filename, InternetDebugFilename, filenameLength);
        } else {
            lpBuffer->Filename[0] = '\0';
        }
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    *lpdwBufferLength = requiredLength;
    return error;
}


DWORD
InternetSetDebugInfo(
    IN LPINTERNET_DEBUG_INFO lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Sets the internal debugging variables to the values in the buffer. To make
    incrmental changes, the caller must first read the variables, change the
    bits they're interested in, then change the whole lot at one go

Arguments:

    lpBuffer        - pointer to structure that contains the variables

    dwBufferLength  - size of lpBuffer. Ignored

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    InternetDebugErrorLevel = lpBuffer->ErrorLevel;
    InternetDebugCategoryFlags = lpBuffer->CategoryFlags;
    InternetDebugControlFlags = lpBuffer->ControlFlags;
    InternetDebugBreakFlags = lpBuffer->BreakFlags;
    InternetDebugIndentIncrement = lpBuffer->IndentIncrement;

    //
    // handle the debug file. If we get an empty string, then (if we are logging
    // to a file), close the file.
    //
    // If the filename is exactly the same as we're already using, then the
    // caller doesn't want to change the log file
    //
    // If the filename is different, then we are being asked to create a new log
    // file: close the old and open the new. If we cannot open the new file then
    // set the filename to the NUL string in the debug buffer
    //

    if (lpBuffer->Filename[0]) {
        if (strcmp(InternetDebugFilename, lpBuffer->Filename) != 0) {
            InternetCloseDebugFile();
            InternetReopenDebugFile(lpBuffer->Filename);
            if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
                strcpy(InternetDebugFilename, lpBuffer->Filename);
            } else {
                lpBuffer->Filename[0] = '\0';
            }
        }
    } else {
        InternetCloseDebugFile();
    }
    return ERROR_SUCCESS;
}


BOOL
InternetOpenDebugFile(
    VOID
    )

/*++

Routine Description:

    Opens debug filename if not already open. Use InternetDebugFilename

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - file was opened

        FALSE   - file not opened (already open or error)

--*/

{
    if (InternetDebugFileHandle == INVALID_HANDLE_VALUE) {
        InternetDebugFileHandle = CreateFile(
            InternetDebugFilename,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,  // lpSecurityAttributes
            (InternetDebugControlFlags & DBG_APPEND_FILE)
                ? OPEN_ALWAYS
                : CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL
            | FILE_FLAG_SEQUENTIAL_SCAN
            | ((InternetDebugControlFlags & DBG_FLUSH_OUTPUT)
                ? FILE_FLAG_WRITE_THROUGH
                : 0),
            NULL
            );
        return InternetDebugFileHandle != INVALID_HANDLE_VALUE;
    }
    return FALSE;
}


BOOL
InternetReopenDebugFile(
    IN LPSTR Filename
    )

/*++

Routine Description:

    (Re)opens a debug log file. Closes the current one if it is open

Arguments:

    Filename    - new file to open

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        InternetCloseDebugFile();
    }
    if (Filename && *Filename) {
        InternetDebugFileHandle = CreateFile(
            Filename,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,  // lpSecurityAttributes
            (InternetDebugControlFlags & DBG_APPEND_FILE)
                ? OPEN_ALWAYS
                : CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL
            | FILE_FLAG_SEQUENTIAL_SCAN
            | ((InternetDebugControlFlags & DBG_FLUSH_OUTPUT)
                ? FILE_FLAG_WRITE_THROUGH
                : 0),
            NULL
            );

        //
        // put our start info in the log file. Mainly useful when we're
        // appending to the file
        //

        if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {

            SYSTEMTIME currentTime;
            char filespec[MAX_PATH + 1];
            LPSTR filename;

            if (GetModuleFileName(NULL, filespec, sizeof(filespec))) {
                filename = strrchr(filespec, '\\');
                if (filename != NULL) {
                    ++filename;
                } else {
                    filename = filespec;
                }
            } else {
                filename = "";
            }

            InternetDebugGetLocalTime(&currentTime, NULL);

            InternetDebugPrintf("\n"
                                ">>>> WinInet Version %d.%d Build %s.%d " __DATE__ " " __TIME__ "\n"
                                ">>>> Process %s [%d (%#x)] started at %02d:%02d:%02d.%03d %02d/%02d/%d\n",
                                InternetVersionInfo.dwMajorVersion,
                                InternetVersionInfo.dwMinorVersion,
                                VER_PRODUCTVERSION_STRING,
                                InternetBuildNumber,
                                filename,
                                GetCurrentProcessId(),
                                GetCurrentProcessId(),
                                currentTime.wHour,
                                currentTime.wMinute,
                                currentTime.wSecond,
                                currentTime.wMilliseconds,
                                currentTime.wMonth,
                                currentTime.wDay,
                                currentTime.wYear
                                );

            InternetDebugPrintf(">>>> Command line = %q\n", GetCommandLine());

            InternetDebugPrintf("\n"
                                "     InternetDebugErrorLevel      = %s [%d]\n"
                                "     InternetDebugControlFlags    = %#08x\n"
                                "     InternetDebugCategoryFlags   = %#08x\n"
                                "     InternetDebugBreakFlags      = %#08x\n"
                                "     InternetDebugIndentIncrement = %d\n"
                                "\n",
                                (InternetDebugErrorLevel == DBG_INFO)       ? "Info"
                                : (InternetDebugErrorLevel == DBG_WARNING)  ? "Warning"
                                : (InternetDebugErrorLevel == DBG_ERROR)    ? "Error"
                                : (InternetDebugErrorLevel == DBG_FATAL)    ? "Fatal"
                                : (InternetDebugErrorLevel == DBG_ALWAYS)   ? "Always"
                                : "?",
                                InternetDebugErrorLevel,
                                InternetDebugControlFlags,
                                InternetDebugCategoryFlags,
                                InternetDebugBreakFlags,
                                InternetDebugIndentIncrement
                                );
            return TRUE;
        }
    }
    return FALSE;
}


VOID
InternetCloseDebugFile(
    VOID
    )

/*++

Routine Description:

    Closes the current debug log file

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        if (InternetDebugControlFlags & DBG_FLUSH_OUTPUT) {
            InternetFlushDebugFile();
        }
        CloseHandle(InternetDebugFileHandle);
        InternetDebugFileHandle = INVALID_HANDLE_VALUE;
    }
}


VOID
InternetFlushDebugFile(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(InternetDebugFileHandle);
    }
}


VOID
InternetDebugSetControlFlags(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Sets debug control flags

Arguments:

    dwFlags - flags to set

Return Value:

    None.

--*/

{
    InternetDebugControlFlags |= dwFlags;
}


VOID
InternetDebugResetControlFlags(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Resets debug control flags

Arguments:

    dwFlags - flags to reset

Return Value:

    None.

--*/

{
    InternetDebugControlFlags &= ~dwFlags;
}


VOID
InternetDebugEnter(
    IN DWORD Category,
    IN DEBUG_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN LPCSTR ParameterList OPTIONAL,
    IN ...
    )

/*++

Routine Description:

    Creates an INTERNET_DEBUG_RECORD for the current function and adds it to
    the per-thread (debug) call-tree

Arguments:

    Category        - category flags, e.g. DBG_FTP

    ReturnType      - type of data it returns

    Function        - name of the function. Must be global, static string

    ParameterList   - string describing parameters to function, or NULL if none

    ...             - parameters to function

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO pThreadInfo;
    LPINTERNET_DEBUG_RECORD pRecord;

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    pThreadInfo = InternetGetThreadInfo();

    //INET_ASSERT(pThreadInfo != NULL);

    if (!pThreadInfo) {
        return;
    }

    pRecord = NEW(INTERNET_DEBUG_RECORD);

    //INET_ASSERT(pRecord != NULL);

    if (!pRecord) {
        return;
    }

    pRecord->Stack = pThreadInfo->Stack;
    pRecord->Category = Category;
    pRecord->ReturnType = ReturnType;
    pRecord->Function = Function;
    pRecord->LastTime = GetTickCount();
    pThreadInfo->Stack = pRecord;
    ++pThreadInfo->CallDepth;

    //
    // if the function's category (FTP, GOPHER, HTTP) is selected in the
    // category flags, then we dump the function entry information
    //

    if (InternetDebugCategoryFlags & Category) {

        char buf[4096];
        LPSTR bufptr;

        bufptr = buf;
        bufptr += rsprintf(bufptr, "%s(", Function);
        if (ARGUMENT_PRESENT(ParameterList)) {

            va_list parms;

            va_start(parms, ParameterList);
            bufptr += _sprintf(bufptr, (char*)ParameterList, parms);
            va_end(parms);
        }
        rsprintf(bufptr, ")\n");
        InternetDebugPrintString(buf);

        //
        // only increase the indentation if we will display debug information
        // for this category
        //

        pThreadInfo->IndentIncrement += InternetDebugIndentIncrement;
    }
}


VOID
InternetDebugLeave(
    IN DWORD_PTR Variable,
    IN LPCSTR Filename,
    IN DWORD LineNumber
    )

/*++

Routine Description:

    Destroys the INTERNET_DEBUG_RECORD for the current function and dumps info
    about what the function is returning, if requested to do so

Arguments:

    Variable    - variable containing value being returned by function

    Filename    - name of file where DEBUG_LEAVE() invoked

    LineNumber  - and line number in Filename

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO pThreadInfo;
    LPINTERNET_DEBUG_RECORD pRecord;
    LPSTR format;
    LPSTR errstr;
    BOOL noVar;
    char formatBuf[128];
    DWORD lastError;
    char hexnumBuf[15];

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    //
    // seems that something in this path can nuke the last error, so we must
    // refresh it
    //

    lastError = GetLastError();

    pThreadInfo = InternetGetThreadInfo();

    //INET_ASSERT(pThreadInfo != NULL);

    if (!pThreadInfo) {
        return;
    }

    pRecord = pThreadInfo->Stack;

    //INET_ASSERT(pRecord != NULL);

    if (!pRecord) {
        return;
    }

    //
    // we are about to output a diagnostic message to the debug log, debugger,
    // or console. First check that we are required to display messages at
    // this level. The level for function ENTER and LEAVE is INFO
    //

    if (InternetDebugErrorLevel >= DBG_INFO) {

        //
        // only display the string and reduce the indent if we are requested
        // for information about this category
        //

        errstr = NULL;
        noVar = FALSE;
        if (InternetDebugCategoryFlags & pRecord->Category) {
            switch (pRecord->ReturnType) {
            case None:
                format = "%s() returning VOID";
                noVar = TRUE;
                break;

            case Bool:
                Variable = (DWORD_PTR)(Variable ? "TRUE" : "FALSE");

                //
                // *** FALL THROUGH ***
                //

            case String:
                format = "%s() returning %s";
                break;

            case Int:
                format = "%s() returning %d";
                break;

            case Dword:
                format = "%s() returning %u";
                errstr = InternetMapError((DWORD)Variable);
                if (errstr != NULL) {
                    if (*errstr == '?') {
                        rsprintf(hexnumBuf, "%#x", Variable);
                        errstr = hexnumBuf;
                        format = "%s() returning %u [?] (%s)";
                    } else {
                        format = "%s() returning %u [%s]";
                    }
                }
                break;

            case Handle:
            case Pointer:
                if (Variable == 0) {
                    format = "%s() returning NULL";
                    noVar = TRUE;
                } else {
                    if (pRecord->ReturnType == Handle) {
                        format = "%s() returning handle %#x";
                    } else {
                        format = "%s() returning %#x";
                    }
                }
                break;

            default:

                INET_ASSERT(FALSE);

                break;
            }

            pThreadInfo->IndentIncrement -= InternetDebugIndentIncrement;
            if (pThreadInfo->IndentIncrement < 0) {
                pThreadInfo->IndentIncrement = 0;
            }

            //
            // add line number info, if requested
            //

            strcpy(formatBuf, format);
            if (!(InternetDebugControlFlags & DBG_NO_LINE_NUMBER)) {
                strcat(formatBuf, " (line %d)");
            }
            strcat(formatBuf, "\n");

            //
            // output an empty line if we are required to separate API calls in
            // the log. Only do this if this is an API level function, and it
            // is the top-level function
            //

            if ((InternetDebugControlFlags & DBG_SEPARATE_APIS)
            && (pRecord->Stack == NULL)) {
                strcat(formatBuf, "\n");
            }

            //
            // dump the line, depending on requirements and number of arguments
            //

            if (noVar) {
                InternetDebugPrint(formatBuf,
                                   pRecord->Function,
                                   LineNumber
                                   );
            } else if (errstr != NULL) {
                InternetDebugPrint(formatBuf,
                                   pRecord->Function,
                                   Variable,
                                   errstr,
                                   LineNumber
                                   );
            } else {
                InternetDebugPrint(formatBuf,
                                   pRecord->Function,
                                   Variable,
                                   LineNumber
                                   );
            }
/*
            //
            // output an empty line if we are required to separate API calls in
            // the log. Only do this if this is an API level function, and it
            // is the top-level function
            //

            if ((InternetDebugControlFlags & DBG_SEPARATE_APIS)
            && (pRecord->Stack == NULL)) {

                //
                // don't call InternetDebugPrint - we don't need timing, thread,
                // level etc. information just for the separator
                //

                InternetDebugOut("\n", FALSE);
            }
*/
        }
    }

    //
    // regardless of whether we are outputting debug info for this category,
    // remove the debug record and reduce the call-depth
    //

    --pThreadInfo->CallDepth;
    pThreadInfo->Stack = pRecord->Stack;

    DEL(pRecord);

    //
    // refresh the last error, in case it was nuked
    //

    SetLastError(lastError);
}


VOID
InternetDebugError(
    IN DWORD Error
    )

/*++

Routine Description:

    Used to display that a function is returning an error. We try to display a
    symbolic name for the error too (as when we are returning a DWORD from a
    function, using DEBUG_LEAVE)

    Displays a string of the form:

        Foo() returning error 87 [ERROR_INVALID_PARAMETER]

Arguments:

    Error   - the error code

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO pThreadInfo;
    LPINTERNET_DEBUG_RECORD pRecord;
    LPSTR errstr;
    DWORD lastError;
    char hexnumBuf[15];

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    //
    // seems that something in this path can nuke the last error, so we must
    // refresh it
    //

    lastError = GetLastError();

    pThreadInfo = InternetGetThreadInfo();

    //INET_ASSERT(pThreadInfo != NULL);
    INET_ASSERT(GetLastError() == lastError);

    if (pThreadInfo == NULL) {
        return;
    }

    pRecord = pThreadInfo->Stack;

    //INET_ASSERT(pRecord != NULL);

    if (pRecord == NULL) {
        return;
    }

    errstr = InternetMapError(Error);
    if ((errstr == NULL) || (*errstr == '?')) {
        rsprintf(hexnumBuf, "%#x", Error);
        errstr = hexnumBuf;
    }
    InternetDebugPrint("%s() returning %d [%s]\n",
                       pRecord->Function,
                       Error,
                       errstr
                       );

    //
    // refresh the last error, in case it was nuked
    //

    SetLastError(lastError);
}


VOID
InternetDebugPrint(
    IN LPSTR Format,
    ...
    )

/*++

Routine Description:

    Internet equivalent of printf()

Arguments:

    Format  - printf format string

    ...     - any extra args

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    //
    // now append the string that the DEBUG_PRINT originally gave us
    //

    va_list list;

    va_start(list, Format);
    _sprintf(bufptr, Format, list);
    va_end(list);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugPrintValist(
    IN LPSTR Format,
    va_list list
    )

/*++

Routine Description:

    Internet equivalent of printf(), but takes valist as the args

Arguments:

    Format  - printf format string

    list    - stack frame of variable arguments

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    _sprintf(bufptr, Format, list);

    InternetDebugOut(buf, FALSE);
}


PRIVATE
VOID
InternetDebugPrintString(
    IN LPSTR String
    )

/*++

Routine Description:

    Same as InternetDebugPrint(), except we perform no expansion on the string

Arguments:

    String  - already formatted string (may contain %s)

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    //
    // now append the string that the DEBUG_PRINT originally gave us
    //

    strcpy(bufptr, String);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugPrintf(
    IN LPSTR Format,
    IN ...
    )

/*++

Routine Description:

    Same as InternetDebugPrint(), but we don't access the per-thread info
    (because we may not have any)

Arguments:

    Format  - printf format string

    ...     - any extra args


Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    va_list list;
    char buf[PRINTF_STACK_BUFFER_LENGTH];

    va_start(list, Format);
    _sprintf(buf, Format, list);
    va_end(list);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugOut(
    IN LPSTR Buffer,
    IN BOOL Assert
    )

/*++

Routine Description:

    Writes a string somewhere - to the debug log file, to the console, or via
    the debugger, or any combination

Arguments:

    Buffer  - pointer to formatted buffer to write

    Assert  - TRUE if this function is being called from InternetAssert(), in
              which case we *always* write to the debugger. Of course, there
              may be no debugger attached, in which case no action is taken

Return Value:

    None.

--*/

{
    int buflen;
    DWORD written;

    buflen = strlen(Buffer);
    if ((InternetDebugControlFlags & DBG_TO_FILE)
    && (InternetDebugFileHandle != INVALID_HANDLE_VALUE)) {
        WriteFile(InternetDebugFileHandle, Buffer, buflen, &written, NULL);
        if (InternetDebugControlFlags & DBG_FLUSH_OUTPUT) {
            InternetFlushDebugFile();
        }
    }

    if (InternetDebugControlFlags & DBG_TO_CONSOLE) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                     Buffer,
                     buflen,
                     &written,
                     0
                     );
    }

    if (Assert || (InternetDebugControlFlags & DBG_TO_DEBUGGER)) {
        OutputDebugString(Buffer);
    }
}


VOID
InternetDebugDump(
    IN LPSTR Text,
    IN LPBYTE Address,
    IN DWORD Size
    )

/*++

Routine Description:

    Dumps Size bytes at Address, in the time-honoured debug tradition

Arguments:

    Text    - to display before dumping data

    Address - start of buffer

    Size    - number of bytes

Return Value:

    None.

--*/

{
    //
    // if flags say no data dumps then quit
    //

    if (InternetDebugControlFlags & (DBG_NO_DEBUG | DBG_NO_DATA_DUMP)) {
        return;
    }

    //
    // display the introduction text, if any
    //

    if (Text) {
        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrint(Text);
        } else {
            InternetDebugOut(Text, FALSE);
        }
    }

    char buf[128];

    //
    // display a line telling us how much data there is, if requested to
    //

    if (InternetDebugControlFlags & DBG_DUMP_LENGTH) {
        rsprintf(buf, "%d (%#x) bytes @ %#x\n", Size, Size, Address);
        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrintString(buf);
        } else {
            InternetDebugOut(buf, FALSE);
        }
    }

    //
    // dump out the data, debug style
    //

    while (Size) {

        int len = InternetDebugDumpFormat(Address, Size, sizeof(BYTE), buf);

        //
        // if we are to indent the data to the current level, then display the
        // buffer via InternetDebugPrint() which will apply all the thread id,
        // indentation, and other options selected, else just display the data
        // via InternetDebugOut(), which will simply send it to the output media
        //

        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrintString(buf);
        } else {
            InternetDebugOut(buf, FALSE);
        }

        Address += len;
        Size -= len;
    }
}


DWORD
InternetDebugDumpFormat(
    IN LPBYTE Address,
    IN DWORD Size,
    IN DWORD ElementSize,
    OUT LPSTR Buffer
    )

/*++

Routine Description:

    Formats Size bytes at Address, in the time-honoured debug tradition, for
    data dump purposes

Arguments:

    Address     - start of buffer

    Size        - number of bytes

    ElementSize - size of each word element in bytes

    Buffer      - pointer to output buffer, assumed to be large enough

Return Value:

    DWORD   - number of bytes formatted

--*/

{
    //
    // we (currently) only understand DWORD, WORD and BYTE dumps
    //

    if ((ElementSize != sizeof(DWORD)) && (ElementSize != sizeof(WORD))) {
        ElementSize = sizeof(BYTE);
    }

    static char spaces[] = "                                               ";    // 15 * 3 + 2
    int i, len;

    len = min(Size, 16);
    rsprintf(Buffer, "%08x  ", Address);

    //
    // dump the hex representation of each character or word - up to 16 per line
    //

    DWORD offset = 10;

    for (i = 0; i < len; i += ElementSize) {

        DWORD value;
        LPSTR formatString;

        switch (ElementSize) {
        case 4:
            formatString = "%08x ";
            value = *(LPDWORD)&Address[i];
            break;

        case 2:
            formatString = "%04x ";
            value = *(LPWORD)&Address[i] & 0xffff;
            break;

        default:
            formatString = ((i & 15) == 7) ? "%02.2x-" : "%02.2x ";
            value = Address[i] & 0xff;
            break;
        }
        rsprintf(&Buffer[offset], formatString, value);
        offset += ElementSize * 2 + 1;
    }

    //
    // write as many spaces as required to tab to ASCII field
    //

    memcpy(&Buffer[offset], spaces, (16 - len) * 3 + 2);
    offset += (16 - len) * 3 + 2;

    //
    // dump ASCII representation of each character
    //

    for (i = 0; i < len; ++i) {

        char ch;

        ch = Address[i];
        Buffer[offset + i] =  ((ch < 32) || (ch > 127)) ? '.' : ch;
    }

    Buffer[offset + i++] = '\r';
    Buffer[offset + i++] = '\n';
    Buffer[offset + i] = '\0';

    return len;
}


VOID
InternetAssert(
    IN LPSTR Assertion,
    IN LPSTR FileName,
    IN DWORD LineNumber
    )

/*++

Routine Description:

    displays assertion message at debugger and raised breakpoint exception

Arguments:

    Assertion   - string describing assertion which failed

    FileName    - module where assertion failure occurred

    LineNumber  - at this line number

Return Value:

    None.

--*/

{
    char buffer[512];

    rsprintf(buffer,
             "\n"
             "*** Wininet Assertion failed: %s\n"
             "*** Source file: %s\n"
             "*** Source line: %d\n"
             "*** Thread %08x\n"
             "\n",
             Assertion,
             FileName,
             LineNumber,
             GetCurrentThreadId()
             );
    InternetDebugOut(buffer, TRUE);

    //
    // break to the debugger, unless it is requested that we don't
    //

    if (!(InternetDebugControlFlags & DBG_NO_ASSERT_BREAK)) {
        DebugBreak();
    }
}


VOID
InternetGetDebugVariable(
    IN LPSTR lpszVariableName,
    OUT LPDWORD lpdwVariable
    )

/*++

Routine Description:

    Get debug variable. First examine environment, then registry

Arguments:

    lpszVariableName    - variable name

    lpdwVariable        - returned variable

Return Value:

    None.

--*/

{
    DWORD len;
    char varbuf[ENVIRONMENT_VARIABLE_BUFFER_LENGTH];

    //
    // get the debug variables first from the environment, then - if not there -
    // from the registry
    //

    len = GetEnvironmentVariable(lpszVariableName, varbuf, sizeof(varbuf));
    if (len && len < sizeof(varbuf)) {
        *lpdwVariable = (DWORD)strtoul(varbuf, NULL, 0);
    } else {
        InternetReadRegistryDword(lpszVariableName, lpdwVariable);
    }
}


PRIVATE
VOID
InternetGetDebugVariableString(
    IN LPSTR lpszVariableName,
    OUT LPSTR lpszVariable,
    IN DWORD dwVariableLen
    )

/*++

Routine Description:

    Get debug variable string. First examine environment, then registry

Arguments:

    lpszVariableName    - variable name

    lpszVariable        - returned string variable

    dwVariableLen       - size of buffer

Return Value:

    None.

--*/

{
    if (GetEnvironmentVariable(lpszVariableName, lpszVariable, dwVariableLen) == 0) {

        char buf[MAX_PATH + 1];
        DWORD len = min(sizeof(buf), dwVariableLen);

        if (InternetReadRegistryString(lpszVariableName, buf, &len) == ERROR_SUCCESS) {
            memcpy(lpszVariable, buf, len + 1);
        }
    }
}

LPSTR
InternetMapChunkToken(
    IN CHUNK_TOKEN ctToken
    )
{
    switch(ctToken)
    {
        CASE_OF(CHUNK_TOKEN_DIGIT);
        CASE_OF(CHUNK_TOKEN_DATA);
        CASE_OF(CHUNK_TOKEN_COLON);
        CASE_OF(CHUNK_TOKEN_CR);
        CASE_OF(CHUNK_TOKEN_LF);
        CASE_OF(CHUNK_TOKEN_INVALID);

        default:
            return "?";

    }
}

LPSTR
InternetMapChunkState(
    IN CHUNK_STATE csState
    )
{
    switch(csState)
    {
        CASE_OF(CHUNK_STATE_START);
        CASE_OF(CHUNK_STATE_SIZE_PARSE);
        CASE_OF(CHUNK_STATE_SIZE_CRLF);
        CASE_OF(CHUNK_STATE_DATA_PARSE);
        CASE_OF(CHUNK_STATE_DATA_CRLF);
        CASE_OF(CHUNK_STATE_ZERO_FOOTER);
        CASE_OF(CHUNK_STATE_ZERO_FOOTER_NAME);
        CASE_OF(CHUNK_STATE_ZERO_FOOTER_VALUE);
        CASE_OF(CHUNK_STATE_ZERO_FOOTER_CRLF);
        CASE_OF(CHUNK_STATE_ZERO_FOOTER_FINAL_CRLF);
        CASE_OF(CHUNK_STATE_FINISHED);

        default:
            return "?";

    }
}


LPSTR
InternetMapError(
    IN DWORD Error
    )

/*++

Routine Description:

    Map error code to string. Try to get all errors that might ever be returned
    by an Internet function

Arguments:

    Error   - code to map

Return Value:

    LPSTR - pointer to symbolic error name

--*/

{
    switch (Error) {

    //
    // WINERROR errors
    //

    CASE_OF(ERROR_SUCCESS);
    CASE_OF(ERROR_INVALID_FUNCTION);
    CASE_OF(ERROR_FILE_NOT_FOUND);
    CASE_OF(ERROR_PATH_NOT_FOUND);
    CASE_OF(ERROR_TOO_MANY_OPEN_FILES);
    CASE_OF(ERROR_ACCESS_DENIED);
    CASE_OF(ERROR_INVALID_HANDLE);
    CASE_OF(ERROR_ARENA_TRASHED);
    CASE_OF(ERROR_NOT_ENOUGH_MEMORY);
    CASE_OF(ERROR_INVALID_BLOCK);
    CASE_OF(ERROR_BAD_ENVIRONMENT);
    CASE_OF(ERROR_BAD_FORMAT);
    CASE_OF(ERROR_INVALID_ACCESS);
    CASE_OF(ERROR_INVALID_DATA);
    CASE_OF(ERROR_OUTOFMEMORY);
    CASE_OF(ERROR_INVALID_DRIVE);
    CASE_OF(ERROR_CURRENT_DIRECTORY);
    CASE_OF(ERROR_NOT_SAME_DEVICE);
    CASE_OF(ERROR_NO_MORE_FILES);
    CASE_OF(ERROR_WRITE_PROTECT);
    CASE_OF(ERROR_BAD_UNIT);
    CASE_OF(ERROR_NOT_READY);
    CASE_OF(ERROR_BAD_COMMAND);
    CASE_OF(ERROR_CRC);
    CASE_OF(ERROR_BAD_LENGTH);
    CASE_OF(ERROR_SEEK);
    CASE_OF(ERROR_NOT_DOS_DISK);
    CASE_OF(ERROR_SECTOR_NOT_FOUND);
    CASE_OF(ERROR_OUT_OF_PAPER);
    CASE_OF(ERROR_WRITE_FAULT);
    CASE_OF(ERROR_READ_FAULT);
    CASE_OF(ERROR_GEN_FAILURE);
    CASE_OF(ERROR_SHARING_VIOLATION);
    CASE_OF(ERROR_LOCK_VIOLATION);
    CASE_OF(ERROR_WRONG_DISK);
    CASE_OF(ERROR_SHARING_BUFFER_EXCEEDED);
    CASE_OF(ERROR_HANDLE_EOF);
    CASE_OF(ERROR_HANDLE_DISK_FULL);
    CASE_OF(ERROR_NOT_SUPPORTED);
    CASE_OF(ERROR_REM_NOT_LIST);
    CASE_OF(ERROR_DUP_NAME);
    CASE_OF(ERROR_BAD_NETPATH);
    CASE_OF(ERROR_NETWORK_BUSY);
    CASE_OF(ERROR_DEV_NOT_EXIST);
    CASE_OF(ERROR_TOO_MANY_CMDS);
    CASE_OF(ERROR_ADAP_HDW_ERR);
    CASE_OF(ERROR_BAD_NET_RESP);
    CASE_OF(ERROR_UNEXP_NET_ERR);
    CASE_OF(ERROR_BAD_REM_ADAP);
    CASE_OF(ERROR_PRINTQ_FULL);
    CASE_OF(ERROR_NO_SPOOL_SPACE);
    CASE_OF(ERROR_PRINT_CANCELLED);
    CASE_OF(ERROR_NETNAME_DELETED);
    CASE_OF(ERROR_NETWORK_ACCESS_DENIED);
    CASE_OF(ERROR_BAD_DEV_TYPE);
    CASE_OF(ERROR_BAD_NET_NAME);
    CASE_OF(ERROR_TOO_MANY_NAMES);
    CASE_OF(ERROR_TOO_MANY_SESS);
    CASE_OF(ERROR_SHARING_PAUSED);
    CASE_OF(ERROR_REQ_NOT_ACCEP);
    CASE_OF(ERROR_REDIR_PAUSED);
    CASE_OF(ERROR_FILE_EXISTS);
    CASE_OF(ERROR_CANNOT_MAKE);
    CASE_OF(ERROR_FAIL_I24);
    CASE_OF(ERROR_OUT_OF_STRUCTURES);
    CASE_OF(ERROR_ALREADY_ASSIGNED);
    CASE_OF(ERROR_INVALID_PASSWORD);
    CASE_OF(ERROR_INVALID_PARAMETER);
    CASE_OF(ERROR_NET_WRITE_FAULT);
    CASE_OF(ERROR_NO_PROC_SLOTS);
    CASE_OF(ERROR_TOO_MANY_SEMAPHORES);
    CASE_OF(ERROR_EXCL_SEM_ALREADY_OWNED);
    CASE_OF(ERROR_SEM_IS_SET);
    CASE_OF(ERROR_TOO_MANY_SEM_REQUESTS);
    CASE_OF(ERROR_INVALID_AT_INTERRUPT_TIME);
    CASE_OF(ERROR_SEM_OWNER_DIED);
    CASE_OF(ERROR_SEM_USER_LIMIT);
    CASE_OF(ERROR_DISK_CHANGE);
    CASE_OF(ERROR_DRIVE_LOCKED);
    CASE_OF(ERROR_BROKEN_PIPE);
    CASE_OF(ERROR_OPEN_FAILED);
    CASE_OF(ERROR_BUFFER_OVERFLOW);
    CASE_OF(ERROR_DISK_FULL);
    CASE_OF(ERROR_NO_MORE_SEARCH_HANDLES);
    CASE_OF(ERROR_INVALID_TARGET_HANDLE);
    CASE_OF(ERROR_INVALID_CATEGORY);
    CASE_OF(ERROR_INVALID_VERIFY_SWITCH);
    CASE_OF(ERROR_BAD_DRIVER_LEVEL);
    CASE_OF(ERROR_CALL_NOT_IMPLEMENTED);
    CASE_OF(ERROR_SEM_TIMEOUT);
    CASE_OF(ERROR_INSUFFICIENT_BUFFER);
    CASE_OF(ERROR_INVALID_NAME);
    CASE_OF(ERROR_INVALID_LEVEL);
    CASE_OF(ERROR_NO_VOLUME_LABEL);
    CASE_OF(ERROR_MOD_NOT_FOUND);
    CASE_OF(ERROR_PROC_NOT_FOUND);
    CASE_OF(ERROR_WAIT_NO_CHILDREN);
    CASE_OF(ERROR_CHILD_NOT_COMPLETE);
    CASE_OF(ERROR_DIRECT_ACCESS_HANDLE);
    CASE_OF(ERROR_NEGATIVE_SEEK);
    CASE_OF(ERROR_SEEK_ON_DEVICE);
    CASE_OF(ERROR_DIR_NOT_ROOT);
    CASE_OF(ERROR_DIR_NOT_EMPTY);
    CASE_OF(ERROR_PATH_BUSY);
    CASE_OF(ERROR_SYSTEM_TRACE);
    CASE_OF(ERROR_INVALID_EVENT_COUNT);
    CASE_OF(ERROR_TOO_MANY_MUXWAITERS);
    CASE_OF(ERROR_INVALID_LIST_FORMAT);
    CASE_OF(ERROR_BAD_ARGUMENTS);
    CASE_OF(ERROR_BAD_PATHNAME);
    CASE_OF(ERROR_BUSY);
    CASE_OF(ERROR_CANCEL_VIOLATION);
    CASE_OF(ERROR_ALREADY_EXISTS);
    CASE_OF(ERROR_FILENAME_EXCED_RANGE);
    CASE_OF(ERROR_LOCKED);
    CASE_OF(ERROR_NESTING_NOT_ALLOWED);
    CASE_OF(ERROR_BAD_PIPE);
    CASE_OF(ERROR_PIPE_BUSY);
    CASE_OF(ERROR_NO_DATA);
    CASE_OF(ERROR_PIPE_NOT_CONNECTED);
    CASE_OF(ERROR_MORE_DATA);
    CASE_OF(ERROR_NO_MORE_ITEMS);
    CASE_OF(ERROR_NOT_OWNER);
    CASE_OF(ERROR_PARTIAL_COPY);
    CASE_OF(ERROR_MR_MID_NOT_FOUND);
    CASE_OF(ERROR_INVALID_ADDRESS);
    CASE_OF(ERROR_PIPE_CONNECTED);
    CASE_OF(ERROR_PIPE_LISTENING);
    CASE_OF(ERROR_OPERATION_ABORTED);
    CASE_OF(ERROR_IO_INCOMPLETE);
    CASE_OF(ERROR_IO_PENDING);
    CASE_OF(ERROR_NOACCESS);
    CASE_OF(ERROR_STACK_OVERFLOW);
    CASE_OF(ERROR_INVALID_FLAGS);
    CASE_OF(ERROR_NO_TOKEN);
    CASE_OF(ERROR_BADDB);
    CASE_OF(ERROR_BADKEY);
    CASE_OF(ERROR_CANTOPEN);
    CASE_OF(ERROR_CANTREAD);
    CASE_OF(ERROR_CANTWRITE);
    CASE_OF(ERROR_REGISTRY_RECOVERED);
    CASE_OF(ERROR_REGISTRY_CORRUPT);
    CASE_OF(ERROR_REGISTRY_IO_FAILED);
    CASE_OF(ERROR_NOT_REGISTRY_FILE);
    CASE_OF(ERROR_KEY_DELETED);
    CASE_OF(ERROR_CIRCULAR_DEPENDENCY);
    CASE_OF(ERROR_SERVICE_NOT_ACTIVE);
    CASE_OF(ERROR_DLL_INIT_FAILED);
    CASE_OF(ERROR_CANCELLED);
    CASE_OF(ERROR_BAD_USERNAME);
    CASE_OF(ERROR_LOGON_FAILURE);

    CASE_OF(WAIT_FAILED);
    //CASE_OF(WAIT_ABANDONED_0);
    CASE_OF(WAIT_TIMEOUT);
    CASE_OF(WAIT_IO_COMPLETION);
    //CASE_OF(STILL_ACTIVE);

    CASE_OF(RPC_S_INVALID_STRING_BINDING);
    CASE_OF(RPC_S_WRONG_KIND_OF_BINDING);
    CASE_OF(RPC_S_INVALID_BINDING);
    CASE_OF(RPC_S_PROTSEQ_NOT_SUPPORTED);
    CASE_OF(RPC_S_INVALID_RPC_PROTSEQ);
    CASE_OF(RPC_S_INVALID_STRING_UUID);
    CASE_OF(RPC_S_INVALID_ENDPOINT_FORMAT);
    CASE_OF(RPC_S_INVALID_NET_ADDR);
    CASE_OF(RPC_S_NO_ENDPOINT_FOUND);
    CASE_OF(RPC_S_INVALID_TIMEOUT);
    CASE_OF(RPC_S_OBJECT_NOT_FOUND);
    CASE_OF(RPC_S_ALREADY_REGISTERED);
    CASE_OF(RPC_S_TYPE_ALREADY_REGISTERED);
    CASE_OF(RPC_S_ALREADY_LISTENING);
    CASE_OF(RPC_S_NO_PROTSEQS_REGISTERED);
    CASE_OF(RPC_S_NOT_LISTENING);
    CASE_OF(RPC_S_UNKNOWN_MGR_TYPE);
    CASE_OF(RPC_S_UNKNOWN_IF);
    CASE_OF(RPC_S_NO_BINDINGS);
    CASE_OF(RPC_S_NO_PROTSEQS);
    CASE_OF(RPC_S_CANT_CREATE_ENDPOINT);
    CASE_OF(RPC_S_OUT_OF_RESOURCES);
    CASE_OF(RPC_S_SERVER_UNAVAILABLE);
    CASE_OF(RPC_S_SERVER_TOO_BUSY);
    CASE_OF(RPC_S_INVALID_NETWORK_OPTIONS);
    CASE_OF(RPC_S_NO_CALL_ACTIVE);
    CASE_OF(RPC_S_CALL_FAILED);
    CASE_OF(RPC_S_CALL_FAILED_DNE);
    CASE_OF(RPC_S_PROTOCOL_ERROR);
    CASE_OF(RPC_S_UNSUPPORTED_TRANS_SYN);
    CASE_OF(RPC_S_UNSUPPORTED_TYPE);
    CASE_OF(RPC_S_INVALID_TAG);
    CASE_OF(RPC_S_INVALID_BOUND);
    CASE_OF(RPC_S_NO_ENTRY_NAME);
    CASE_OF(RPC_S_INVALID_NAME_SYNTAX);
    CASE_OF(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    CASE_OF(RPC_S_UUID_NO_ADDRESS);
    CASE_OF(RPC_S_DUPLICATE_ENDPOINT);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_TYPE);
    CASE_OF(RPC_S_MAX_CALLS_TOO_SMALL);
    CASE_OF(RPC_S_STRING_TOO_LONG);
    CASE_OF(RPC_S_PROTSEQ_NOT_FOUND);
    CASE_OF(RPC_S_PROCNUM_OUT_OF_RANGE);
    CASE_OF(RPC_S_BINDING_HAS_NO_AUTH);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_SERVICE);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_LEVEL);
    CASE_OF(RPC_S_INVALID_AUTH_IDENTITY);
    CASE_OF(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    CASE_OF(EPT_S_INVALID_ENTRY);
    CASE_OF(EPT_S_CANT_PERFORM_OP);
    CASE_OF(EPT_S_NOT_REGISTERED);
    CASE_OF(RPC_S_NOTHING_TO_EXPORT);
    CASE_OF(RPC_S_INCOMPLETE_NAME);
    CASE_OF(RPC_S_INVALID_VERS_OPTION);
    CASE_OF(RPC_S_NO_MORE_MEMBERS);
    CASE_OF(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    CASE_OF(RPC_S_INTERFACE_NOT_FOUND);
    CASE_OF(RPC_S_ENTRY_ALREADY_EXISTS);
    CASE_OF(RPC_S_ENTRY_NOT_FOUND);
    CASE_OF(RPC_S_NAME_SERVICE_UNAVAILABLE);
    CASE_OF(RPC_S_INVALID_NAF_ID);
    CASE_OF(RPC_S_CANNOT_SUPPORT);
    CASE_OF(RPC_S_NO_CONTEXT_AVAILABLE);
    CASE_OF(RPC_S_INTERNAL_ERROR);
    CASE_OF(RPC_S_ZERO_DIVIDE);
    CASE_OF(RPC_S_ADDRESS_ERROR);
    CASE_OF(RPC_S_FP_DIV_ZERO);
    CASE_OF(RPC_S_FP_UNDERFLOW);
    CASE_OF(RPC_S_FP_OVERFLOW);
    CASE_OF(RPC_X_NO_MORE_ENTRIES);
    CASE_OF(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    CASE_OF(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    CASE_OF(RPC_X_SS_IN_NULL_CONTEXT);
    CASE_OF(RPC_X_SS_CONTEXT_DAMAGED);
    CASE_OF(RPC_X_SS_HANDLES_MISMATCH);
    CASE_OF(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    CASE_OF(RPC_X_NULL_REF_POINTER);
    CASE_OF(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    CASE_OF(RPC_X_BYTE_COUNT_TOO_SMALL);
    CASE_OF(RPC_X_BAD_STUB_DATA);


    //
    // WININET errors
    //

    CASE_OF(ERROR_INTERNET_OUT_OF_HANDLES);
    CASE_OF(ERROR_INTERNET_TIMEOUT);
    CASE_OF(ERROR_INTERNET_EXTENDED_ERROR);
    CASE_OF(ERROR_INTERNET_INTERNAL_ERROR);
    CASE_OF(ERROR_INTERNET_INVALID_URL);
    CASE_OF(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
    CASE_OF(ERROR_INTERNET_NAME_NOT_RESOLVED);
    CASE_OF(ERROR_INTERNET_PROTOCOL_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_INVALID_OPTION);
    CASE_OF(ERROR_INTERNET_BAD_OPTION_LENGTH);
    CASE_OF(ERROR_INTERNET_OPTION_NOT_SETTABLE);
    CASE_OF(ERROR_INTERNET_SHUTDOWN);
    CASE_OF(ERROR_INTERNET_INCORRECT_USER_NAME);
    CASE_OF(ERROR_INTERNET_INCORRECT_PASSWORD);
    CASE_OF(ERROR_INTERNET_LOGIN_FAILURE);
    CASE_OF(ERROR_INTERNET_INVALID_OPERATION);
    CASE_OF(ERROR_INTERNET_OPERATION_CANCELLED);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_STATE);
    CASE_OF(ERROR_INTERNET_NOT_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_BAD_REGISTRY_PARAMETER);
    CASE_OF(ERROR_INTERNET_NO_DIRECT_ACCESS);
    CASE_OF(ERROR_INTERNET_NO_CONTEXT);
    CASE_OF(ERROR_INTERNET_NO_CALLBACK);
    CASE_OF(ERROR_INTERNET_REQUEST_PENDING);
    CASE_OF(ERROR_INTERNET_INCORRECT_FORMAT);
    CASE_OF(ERROR_INTERNET_ITEM_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_CANNOT_CONNECT);
    CASE_OF(ERROR_INTERNET_CONNECTION_ABORTED);
    CASE_OF(ERROR_INTERNET_CONNECTION_RESET);
    CASE_OF(ERROR_INTERNET_FORCE_RETRY);
    CASE_OF(ERROR_INTERNET_INVALID_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_NEED_UI);
    CASE_OF(ERROR_INTERNET_HANDLE_EXISTS);
    CASE_OF(ERROR_INTERNET_SEC_CERT_DATE_INVALID);
    CASE_OF(ERROR_INTERNET_SEC_CERT_CN_INVALID);
    CASE_OF(ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR);
    CASE_OF(ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR);
    CASE_OF(ERROR_INTERNET_MIXED_SECURITY);
    CASE_OF(ERROR_INTERNET_CHG_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED);
    CASE_OF(ERROR_INTERNET_INVALID_CA);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP);
    CASE_OF(ERROR_INTERNET_ASYNC_THREAD_FAILED);
    CASE_OF(ERROR_INTERNET_REDIRECT_SCHEME_CHANGE);
    CASE_OF(ERROR_INTERNET_DIALOG_PENDING);
    CASE_OF(ERROR_INTERNET_RETRY_DIALOG);
    CASE_OF(ERROR_INTERNET_NO_NEW_CONTAINERS);
    CASE_OF(ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR);
    CASE_OF(ERROR_INTERNET_INSERT_CDROM);
    CASE_OF(ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED);
    CASE_OF(ERROR_INTERNET_SEC_CERT_ERRORS);
    CASE_OF(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_CACHE_FILE);
    CASE_OF(ERROR_INTERNET_TCPIP_NOT_INSTALLED);
    CASE_OF(ERROR_INTERNET_OFFLINE);
    CASE_OF(ERROR_INTERNET_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_PROXY_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT);
    CASE_OF(ERROR_INTERNET_SEC_INVALID_CERT);
    CASE_OF(ERROR_INTERNET_SEC_CERT_REVOKED);
    CASE_OF(ERROR_INTERNET_FAILED_DUETOSECURITYCHECK);
    CASE_OF(ERROR_INTERNET_NOT_INITIALIZED);

    CASE_OF(ERROR_FTP_TRANSFER_IN_PROGRESS);
    CASE_OF(ERROR_FTP_DROPPED);
    CASE_OF(ERROR_FTP_NO_PASSIVE_MODE);

    CASE_OF(ERROR_GOPHER_PROTOCOL_ERROR);
    CASE_OF(ERROR_GOPHER_NOT_FILE);
    CASE_OF(ERROR_GOPHER_DATA_ERROR);
    CASE_OF(ERROR_GOPHER_END_OF_DATA);
    CASE_OF(ERROR_GOPHER_INVALID_LOCATOR);
    CASE_OF(ERROR_GOPHER_INCORRECT_LOCATOR_TYPE);
    CASE_OF(ERROR_GOPHER_NOT_GOPHER_PLUS);
    CASE_OF(ERROR_GOPHER_ATTRIBUTE_NOT_FOUND);
    CASE_OF(ERROR_GOPHER_UNKNOWN_LOCATOR);

    CASE_OF(ERROR_HTTP_HEADER_NOT_FOUND);
    CASE_OF(ERROR_HTTP_DOWNLEVEL_SERVER);
    CASE_OF(ERROR_HTTP_INVALID_SERVER_RESPONSE);
    CASE_OF(ERROR_HTTP_INVALID_HEADER);
    CASE_OF(ERROR_HTTP_INVALID_QUERY_REQUEST);
    CASE_OF(ERROR_HTTP_HEADER_ALREADY_EXISTS);
    CASE_OF(ERROR_HTTP_REDIRECT_FAILED);
    CASE_OF(ERROR_HTTP_NOT_REDIRECTED);
    CASE_OF(ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION);
    CASE_OF(ERROR_HTTP_COOKIE_DECLINED);
    CASE_OF(ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION);

    //
    // internal WININET errors
    //

    CASE_OF(ERROR_INTERNET_INTERNAL_SOCKET_ERROR);
    CASE_OF(ERROR_INTERNET_CONNECTION_AVAILABLE);
    CASE_OF(ERROR_INTERNET_NO_KNOWN_SERVERS);
    CASE_OF(ERROR_INTERNET_PING_FAILED);
    CASE_OF(ERROR_INTERNET_NO_PING_SUPPORT);
    CASE_OF(ERROR_INTERNET_CACHE_SUCCESS);


    //
    // SSPI errors
    //

    CASE_OF(SEC_E_INSUFFICIENT_MEMORY);
    CASE_OF(SEC_E_INVALID_HANDLE);
    CASE_OF(SEC_E_UNSUPPORTED_FUNCTION);
    CASE_OF(SEC_E_TARGET_UNKNOWN);
    CASE_OF(SEC_E_INTERNAL_ERROR);
    CASE_OF(SEC_E_SECPKG_NOT_FOUND);
    CASE_OF(SEC_E_NOT_OWNER);
    CASE_OF(SEC_E_CANNOT_INSTALL);
    CASE_OF(SEC_E_INVALID_TOKEN);
    CASE_OF(SEC_E_CANNOT_PACK);
    CASE_OF(SEC_E_QOP_NOT_SUPPORTED);
    CASE_OF(SEC_E_NO_IMPERSONATION);
    CASE_OF(SEC_E_LOGON_DENIED);
    CASE_OF(SEC_E_UNKNOWN_CREDENTIALS);
    CASE_OF(SEC_E_NO_CREDENTIALS);
    CASE_OF(SEC_E_MESSAGE_ALTERED);
    CASE_OF(SEC_E_OUT_OF_SEQUENCE);
    CASE_OF(SEC_E_NO_AUTHENTICATING_AUTHORITY);
    CASE_OF(SEC_I_CONTINUE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_AND_CONTINUE);
    CASE_OF(SEC_I_LOCAL_LOGON);
    CASE_OF(SEC_E_BAD_PKGID);
    CASE_OF(SEC_E_CONTEXT_EXPIRED);
    CASE_OF(SEC_E_INCOMPLETE_MESSAGE);


    //
    // WINSOCK errors
    //

    CASE_OF(WSAEINTR);
    CASE_OF(WSAEBADF);
    CASE_OF(WSAEACCES);
    CASE_OF(WSAEFAULT);
    CASE_OF(WSAEINVAL);
    CASE_OF(WSAEMFILE);
    CASE_OF(WSAEWOULDBLOCK);
    CASE_OF(WSAEINPROGRESS);
    CASE_OF(WSAEALREADY);
    CASE_OF(WSAENOTSOCK);
    CASE_OF(WSAEDESTADDRREQ);
    CASE_OF(WSAEMSGSIZE);
    CASE_OF(WSAEPROTOTYPE);
    CASE_OF(WSAENOPROTOOPT);
    CASE_OF(WSAEPROTONOSUPPORT);
    CASE_OF(WSAESOCKTNOSUPPORT);
    CASE_OF(WSAEOPNOTSUPP);
    CASE_OF(WSAEPFNOSUPPORT);
    CASE_OF(WSAEAFNOSUPPORT);
    CASE_OF(WSAEADDRINUSE);
    CASE_OF(WSAEADDRNOTAVAIL);
    CASE_OF(WSAENETDOWN);
    CASE_OF(WSAENETUNREACH);
    CASE_OF(WSAENETRESET);
    CASE_OF(WSAECONNABORTED);
    CASE_OF(WSAECONNRESET);
    CASE_OF(WSAENOBUFS);
    CASE_OF(WSAEISCONN);
    CASE_OF(WSAENOTCONN);
    CASE_OF(WSAESHUTDOWN);
    CASE_OF(WSAETOOMANYREFS);
    CASE_OF(WSAETIMEDOUT);
    CASE_OF(WSAECONNREFUSED);
    CASE_OF(WSAELOOP);
    CASE_OF(WSAENAMETOOLONG);
    CASE_OF(WSAEHOSTDOWN);
    CASE_OF(WSAEHOSTUNREACH);
    CASE_OF(WSAENOTEMPTY);
    CASE_OF(WSAEPROCLIM);
    CASE_OF(WSAEUSERS);
    CASE_OF(WSAEDQUOT);
    CASE_OF(WSAESTALE);
    CASE_OF(WSAEREMOTE);
    CASE_OF(WSAEDISCON);
    CASE_OF(WSASYSNOTREADY);
    CASE_OF(WSAVERNOTSUPPORTED);
    CASE_OF(WSANOTINITIALISED);
    CASE_OF(WSAHOST_NOT_FOUND);
    CASE_OF(WSATRY_AGAIN);
    CASE_OF(WSANO_RECOVERY);
    CASE_OF(WSANO_DATA);

#if 0
    //
    // RAS errors
    //

    CASE_OF(PENDING);
    CASE_OF(ERROR_INVALID_PORT_HANDLE);
    CASE_OF(ERROR_PORT_ALREADY_OPEN);
    CASE_OF(ERROR_BUFFER_TOO_SMALL);
    CASE_OF(ERROR_WRONG_INFO_SPECIFIED);
    CASE_OF(ERROR_CANNOT_SET_PORT_INFO);
    CASE_OF(ERROR_PORT_NOT_CONNECTED);
    CASE_OF(ERROR_EVENT_INVALID);
    CASE_OF(ERROR_DEVICE_DOES_NOT_EXIST);
    CASE_OF(ERROR_BUFFER_INVALID);
    CASE_OF(ERROR_ROUTE_NOT_AVAILABLE);
    CASE_OF(ERROR_ROUTE_NOT_ALLOCATED);
    CASE_OF(ERROR_INVALID_COMPRESSION_SPECIFIED);
    CASE_OF(ERROR_OUT_OF_BUFFERS);
    CASE_OF(ERROR_PORT_NOT_FOUND);
    CASE_OF(ERROR_ASYNC_REQUEST_PENDING);
    CASE_OF(ERROR_ALREADY_DISCONNECTING);
    CASE_OF(ERROR_PORT_NOT_OPEN);
    CASE_OF(ERROR_PORT_DISCONNECTED);
    CASE_OF(ERROR_NO_ENDPOINTS);
    CASE_OF(ERROR_CANNOT_OPEN_PHONEBOOK);
    CASE_OF(ERROR_CANNOT_LOAD_PHONEBOOK);
    CASE_OF(ERROR_CANNOT_FIND_PHONEBOOK_ENTRY);
    CASE_OF(ERROR_CANNOT_WRITE_PHONEBOOK);
    CASE_OF(ERROR_CORRUPT_PHONEBOOK);
    CASE_OF(ERROR_CANNOT_LOAD_STRING);
    CASE_OF(ERROR_KEY_NOT_FOUND);
    CASE_OF(ERROR_DISCONNECTION);
    CASE_OF(ERROR_REMOTE_DISCONNECTION);
    CASE_OF(ERROR_HARDWARE_FAILURE);
    CASE_OF(ERROR_USER_DISCONNECTION);
    CASE_OF(ERROR_INVALID_SIZE);
    CASE_OF(ERROR_PORT_NOT_AVAILABLE);
    CASE_OF(ERROR_CANNOT_PROJECT_CLIENT);
    CASE_OF(ERROR_UNKNOWN);
    CASE_OF(ERROR_WRONG_DEVICE_ATTACHED);
    CASE_OF(ERROR_BAD_STRING);
    CASE_OF(ERROR_REQUEST_TIMEOUT);
    CASE_OF(ERROR_CANNOT_GET_LANA);
    CASE_OF(ERROR_NETBIOS_ERROR);
    CASE_OF(ERROR_SERVER_OUT_OF_RESOURCES);
    CASE_OF(ERROR_NAME_EXISTS_ON_NET);
    CASE_OF(ERROR_SERVER_GENERAL_NET_FAILURE);
    CASE_OF(WARNING_MSG_ALIAS_NOT_ADDED);
    CASE_OF(ERROR_AUTH_INTERNAL);
    CASE_OF(ERROR_RESTRICTED_LOGON_HOURS);
    CASE_OF(ERROR_ACCT_DISABLED);
    CASE_OF(ERROR_PASSWD_EXPIRED);
    CASE_OF(ERROR_NO_DIALIN_PERMISSION);
    CASE_OF(ERROR_SERVER_NOT_RESPONDING);
    CASE_OF(ERROR_FROM_DEVICE);
    CASE_OF(ERROR_UNRECOGNIZED_RESPONSE);
    CASE_OF(ERROR_MACRO_NOT_FOUND);
    CASE_OF(ERROR_MACRO_NOT_DEFINED);
    CASE_OF(ERROR_MESSAGE_MACRO_NOT_FOUND);
    CASE_OF(ERROR_DEFAULTOFF_MACRO_NOT_FOUND);
    CASE_OF(ERROR_FILE_COULD_NOT_BE_OPENED);
    CASE_OF(ERROR_DEVICENAME_TOO_LONG);
    CASE_OF(ERROR_DEVICENAME_NOT_FOUND);
    CASE_OF(ERROR_NO_RESPONSES);
    CASE_OF(ERROR_NO_COMMAND_FOUND);
    CASE_OF(ERROR_WRONG_KEY_SPECIFIED);
    CASE_OF(ERROR_UNKNOWN_DEVICE_TYPE);
    CASE_OF(ERROR_ALLOCATING_MEMORY);
    CASE_OF(ERROR_PORT_NOT_CONFIGURED);
    CASE_OF(ERROR_DEVICE_NOT_READY);
    CASE_OF(ERROR_READING_INI_FILE);
    CASE_OF(ERROR_NO_CONNECTION);
    CASE_OF(ERROR_BAD_USAGE_IN_INI_FILE);
    CASE_OF(ERROR_READING_SECTIONNAME);
    CASE_OF(ERROR_READING_DEVICETYPE);
    CASE_OF(ERROR_READING_DEVICENAME);
    CASE_OF(ERROR_READING_USAGE);
    CASE_OF(ERROR_READING_MAXCONNECTBPS);
    CASE_OF(ERROR_READING_MAXCARRIERBPS);
    CASE_OF(ERROR_LINE_BUSY);
    CASE_OF(ERROR_VOICE_ANSWER);
    CASE_OF(ERROR_NO_ANSWER);
    CASE_OF(ERROR_NO_CARRIER);
    CASE_OF(ERROR_NO_DIALTONE);
    CASE_OF(ERROR_IN_COMMAND);
    CASE_OF(ERROR_WRITING_SECTIONNAME);
    CASE_OF(ERROR_WRITING_DEVICETYPE);
    CASE_OF(ERROR_WRITING_DEVICENAME);
    CASE_OF(ERROR_WRITING_MAXCONNECTBPS);
    CASE_OF(ERROR_WRITING_MAXCARRIERBPS);
    CASE_OF(ERROR_WRITING_USAGE);
    CASE_OF(ERROR_WRITING_DEFAULTOFF);
    CASE_OF(ERROR_READING_DEFAULTOFF);
    CASE_OF(ERROR_EMPTY_INI_FILE);
    CASE_OF(ERROR_AUTHENTICATION_FAILURE);
    CASE_OF(ERROR_PORT_OR_DEVICE);
    CASE_OF(ERROR_NOT_BINARY_MACRO);
    CASE_OF(ERROR_DCB_NOT_FOUND);
    CASE_OF(ERROR_STATE_MACHINES_NOT_STARTED);
    CASE_OF(ERROR_STATE_MACHINES_ALREADY_STARTED);
    CASE_OF(ERROR_PARTIAL_RESPONSE_LOOPING);
    CASE_OF(ERROR_UNKNOWN_RESPONSE_KEY);
    CASE_OF(ERROR_RECV_BUF_FULL);
    CASE_OF(ERROR_CMD_TOO_LONG);
    CASE_OF(ERROR_UNSUPPORTED_BPS);
    CASE_OF(ERROR_UNEXPECTED_RESPONSE);
    CASE_OF(ERROR_INTERACTIVE_MODE);
    CASE_OF(ERROR_BAD_CALLBACK_NUMBER);
    CASE_OF(ERROR_INVALID_AUTH_STATE);
    CASE_OF(ERROR_WRITING_INITBPS);
    CASE_OF(ERROR_X25_DIAGNOSTIC);
    CASE_OF(ERROR_ACCT_EXPIRED);
    CASE_OF(ERROR_CHANGING_PASSWORD);
    CASE_OF(ERROR_OVERRUN);
    CASE_OF(ERROR_RASMAN_CANNOT_INITIALIZE);
    CASE_OF(ERROR_BIPLEX_PORT_NOT_AVAILABLE);
    CASE_OF(ERROR_NO_ACTIVE_ISDN_LINES);
    CASE_OF(ERROR_NO_ISDN_CHANNELS_AVAILABLE);
    CASE_OF(ERROR_TOO_MANY_LINE_ERRORS);
    CASE_OF(ERROR_IP_CONFIGURATION);
    CASE_OF(ERROR_NO_IP_ADDRESSES);
    CASE_OF(ERROR_PPP_TIMEOUT);
    CASE_OF(ERROR_PPP_REMOTE_TERMINATED);
    CASE_OF(ERROR_PPP_NO_PROTOCOLS_CONFIGURED);
    CASE_OF(ERROR_PPP_NO_RESPONSE);
    CASE_OF(ERROR_PPP_INVALID_PACKET);
    CASE_OF(ERROR_PHONE_NUMBER_TOO_LONG);
    CASE_OF(ERROR_IPXCP_NO_DIALOUT_CONFIGURED);
    CASE_OF(ERROR_IPXCP_NO_DIALIN_CONFIGURED);
    CASE_OF(ERROR_IPXCP_DIALOUT_ALREADY_ACTIVE);
    CASE_OF(ERROR_ACCESSING_TCPCFGDLL);
    CASE_OF(ERROR_NO_IP_RAS_ADAPTER);
    CASE_OF(ERROR_SLIP_REQUIRES_IP);
    CASE_OF(ERROR_PROJECTION_NOT_COMPLETE);
    CASE_OF(ERROR_PROTOCOL_NOT_CONFIGURED);
    CASE_OF(ERROR_PPP_NOT_CONVERGING);
    CASE_OF(ERROR_PPP_CP_REJECTED);
    CASE_OF(ERROR_PPP_LCP_TERMINATED);
    CASE_OF(ERROR_PPP_REQUIRED_ADDRESS_REJECTED);
    CASE_OF(ERROR_PPP_NCP_TERMINATED);
    CASE_OF(ERROR_PPP_LOOPBACK_DETECTED);
    CASE_OF(ERROR_PPP_NO_ADDRESS_ASSIGNED);
    CASE_OF(ERROR_CANNOT_USE_LOGON_CREDENTIALS);
    CASE_OF(ERROR_TAPI_CONFIGURATION);
    CASE_OF(ERROR_NO_LOCAL_ENCRYPTION);
    CASE_OF(ERROR_NO_REMOTE_ENCRYPTION);
    CASE_OF(ERROR_REMOTE_REQUIRES_ENCRYPTION);
    CASE_OF(ERROR_IPXCP_NET_NUMBER_CONFLICT);
    CASE_OF(ERROR_INVALID_SMM);
    CASE_OF(ERROR_SMM_UNINITIALIZED);
    CASE_OF(ERROR_NO_MAC_FOR_PORT);
    CASE_OF(ERROR_SMM_TIMEOUT);
    CASE_OF(ERROR_BAD_PHONE_NUMBER);
    CASE_OF(ERROR_WRONG_MODULE);
    CASE_OF(ERROR_INVALID_CALLBACK_NUMBER);
    CASE_OF(ERROR_SCRIPT_SYNTAX);
#endif // 0
    default:
        return "?";
    }
}


LPSTR
InternetMapStatus(
    IN DWORD Status
    )

/*++

Routine Description:

    Convert INTERNET_STATUS_ value to symbolic name

Arguments:

    Status  - to map

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Status) {
    CASE_OF(INTERNET_STATUS_RESOLVING_NAME);
    CASE_OF(INTERNET_STATUS_NAME_RESOLVED);
    CASE_OF(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CASE_OF(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CASE_OF(INTERNET_STATUS_SENDING_REQUEST);
    CASE_OF(INTERNET_STATUS_REQUEST_SENT);
    CASE_OF(INTERNET_STATUS_RECEIVING_RESPONSE);
    CASE_OF(INTERNET_STATUS_RESPONSE_RECEIVED);
    CASE_OF(INTERNET_STATUS_CTL_RESPONSE_RECEIVED);
    CASE_OF(INTERNET_STATUS_PREFETCH);
    CASE_OF(INTERNET_STATUS_CLOSING_CONNECTION);
    CASE_OF(INTERNET_STATUS_CONNECTION_CLOSED);
    CASE_OF(INTERNET_STATUS_HANDLE_CREATED);
    CASE_OF(INTERNET_STATUS_HANDLE_CLOSING);
    CASE_OF(INTERNET_STATUS_REQUEST_COMPLETE);
    CASE_OF(INTERNET_STATUS_REDIRECT);
    CASE_OF(INTERNET_STATUS_INTERMEDIATE_RESPONSE);
    CASE_OF(INTERNET_STATUS_STATE_CHANGE);
    CASE_OF(INTERNET_STATUS_USER_INPUT_REQUIRED);
    }
    return "?";
}


LPSTR
InternetMapSSPIError(
    IN DWORD Status
    )

/*++

Routine Description:

    Convert a SSL/PCT SSPI Error Code to a string.

Arguments:

    Status  - to map

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Status) {

    CASE_OF(STATUS_SUCCESS);
    CASE_OF(SEC_E_INSUFFICIENT_MEMORY        );
    CASE_OF(SEC_E_INVALID_HANDLE             );
    CASE_OF(SEC_E_UNSUPPORTED_FUNCTION       );
    CASE_OF(SEC_E_TARGET_UNKNOWN             );
    CASE_OF(SEC_E_INTERNAL_ERROR             );
    CASE_OF(SEC_E_SECPKG_NOT_FOUND           );
    CASE_OF(SEC_E_NOT_OWNER                  );
    CASE_OF(SEC_E_CANNOT_INSTALL             );
    CASE_OF(SEC_E_INVALID_TOKEN              );
    CASE_OF(SEC_E_CANNOT_PACK                );
    CASE_OF(SEC_E_QOP_NOT_SUPPORTED          );
    CASE_OF(SEC_E_NO_IMPERSONATION           );
    CASE_OF(SEC_E_LOGON_DENIED               );
    CASE_OF(SEC_E_UNKNOWN_CREDENTIALS        );
    CASE_OF(SEC_E_NO_CREDENTIALS             );
    CASE_OF(SEC_E_MESSAGE_ALTERED            );
    CASE_OF(SEC_E_OUT_OF_SEQUENCE            );
    CASE_OF(SEC_E_NO_AUTHENTICATING_AUTHORITY);
    CASE_OF(SEC_I_CONTINUE_NEEDED            );
    CASE_OF(SEC_I_COMPLETE_NEEDED            );
    CASE_OF(SEC_I_COMPLETE_AND_CONTINUE      );
    CASE_OF(SEC_I_LOCAL_LOGON                );
    CASE_OF(SEC_E_BAD_PKGID                  );
    CASE_OF(SEC_E_CONTEXT_EXPIRED            );
    CASE_OF(SEC_E_INCOMPLETE_MESSAGE         );
//    CASE_OF(SEC_E_NO_SPM                     );
//    CASE_OF(SEC_E_NOT_SUPPORTED              );

    }
    return "?";
}




LPSTR
InternetMapOption(
    IN DWORD Option
    )

/*++

Routine Description:

    Convert INTERNET_OPTION_ value to symbolic name

Arguments:

    Option  - to map

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Option) {
    CASE_OF(INTERNET_OPTION_CALLBACK);
    CASE_OF(INTERNET_OPTION_CONNECT_TIMEOUT);
    CASE_OF(INTERNET_OPTION_CONNECT_RETRIES);
    CASE_OF(INTERNET_OPTION_CONNECT_BACKOFF);
    CASE_OF(INTERNET_OPTION_SEND_TIMEOUT);
    CASE_OF(INTERNET_OPTION_RECEIVE_TIMEOUT);
    CASE_OF(INTERNET_OPTION_DATA_SEND_TIMEOUT);
    CASE_OF(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT);
    CASE_OF(INTERNET_OPTION_HANDLE_TYPE);
    CASE_OF(INTERNET_OPTION_CONTEXT_VALUE_OLD);
    CASE_OF(INTERNET_OPTION_NAME_RES_THREAD);
    CASE_OF(INTERNET_OPTION_READ_BUFFER_SIZE);
    CASE_OF(INTERNET_OPTION_WRITE_BUFFER_SIZE);
    CASE_OF(INTERNET_OPTION_GATEWAY_NAME);
    CASE_OF(INTERNET_OPTION_ASYNC_ID);
    CASE_OF(INTERNET_OPTION_ASYNC_PRIORITY);
    CASE_OF(INTERNET_OPTION_ASYNC_REQUEST_COUNT);
    CASE_OF(INTERNET_OPTION_MAXIMUM_WORKER_THREADS);
    CASE_OF(INTERNET_OPTION_ASYNC_QUEUE_DEPTH);
    CASE_OF(INTERNET_OPTION_WORKER_THREAD_TIMEOUT);
    CASE_OF(INTERNET_OPTION_PARENT_HANDLE);
    CASE_OF(INTERNET_OPTION_KEEP_CONNECTION);
    CASE_OF(INTERNET_OPTION_REQUEST_FLAGS);
    CASE_OF(INTERNET_OPTION_EXTENDED_ERROR);
    CASE_OF(INTERNET_OPTION_RECEIVE_ALL_MODE);
    CASE_OF(INTERNET_OPTION_OFFLINE_MODE);
    CASE_OF(INTERNET_OPTION_CACHE_STREAM_HANDLE);
    CASE_OF(INTERNET_OPTION_USERNAME);
    CASE_OF(INTERNET_OPTION_PASSWORD);
    CASE_OF(INTERNET_OPTION_ASYNC);
    CASE_OF(INTERNET_OPTION_SECURITY_FLAGS);
    CASE_OF(INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT);
    CASE_OF(INTERNET_OPTION_DATAFILE_NAME);
    CASE_OF(INTERNET_OPTION_URL);
    CASE_OF(INTERNET_OPTION_SECURITY_CERTIFICATE);
    CASE_OF(INTERNET_OPTION_SECURITY_KEY_BITNESS);
    CASE_OF(INTERNET_OPTION_REFRESH);
    CASE_OF(INTERNET_OPTION_PROXY);
    CASE_OF(INTERNET_OPTION_SETTINGS_CHANGED);
    CASE_OF(INTERNET_OPTION_VERSION);
    CASE_OF(INTERNET_OPTION_USER_AGENT);
    CASE_OF(INTERNET_OPTION_END_BROWSER_SESSION);
    CASE_OF(INTERNET_OPTION_PROXY_USERNAME);
    CASE_OF(INTERNET_OPTION_PROXY_PASSWORD);
    CASE_OF(INTERNET_OPTION_CONTEXT_VALUE);
    CASE_OF(INTERNET_OPTION_CONNECT_LIMIT);
    CASE_OF(INTERNET_OPTION_SECURITY_SELECT_CLIENT_CERT);
    CASE_OF(INTERNET_OPTION_POLICY);
    CASE_OF(INTERNET_OPTION_OFFLINE_TIMEOUT);
    CASE_OF(INTERNET_OPTION_LINE_STATE);
    CASE_OF(INTERNET_OPTION_IDLE_STATE);
    CASE_OF(INTERNET_OPTION_OFFLINE_SEMANTICS);
    CASE_OF(INTERNET_OPTION_SECONDARY_CACHE_KEY);
    CASE_OF(INTERNET_OPTION_CALLBACK_FILTER);
    CASE_OF(INTERNET_OPTION_CONNECT_TIME);
    CASE_OF(INTERNET_OPTION_SEND_THROUGHPUT);
    CASE_OF(INTERNET_OPTION_REQUEST_PRIORITY);
    CASE_OF(INTERNET_OPTION_HTTP_VERSION);
    CASE_OF(INTERNET_OPTION_RESET_URLCACHE_SESSION);
    CASE_OF(INTERNET_OPTION_NET_SPEED);
    CASE_OF(INTERNET_OPTION_ERROR_MASK);
    CASE_OF(INTERNET_OPTION_FROM_CACHE_TIMEOUT);
    CASE_OF(INTERNET_OPTION_BYPASS_EDITED_ENTRY);
    CASE_OF(INTERNET_OPTION_SECURITY_CONNECTION_INFO);
    CASE_OF(INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO);
    }
    return "?";
}


LPSTR
InternetMapHttpOption(
    IN DWORD Option
    )

/*++

Routine Description:

    Convert HTTP_QUERY_ option value to symbolic name

Arguments:

    Option  - to map

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Option) {
    CASE_OF(HTTP_QUERY_MIME_VERSION);               // 0
    CASE_OF(HTTP_QUERY_CONTENT_TYPE);               // 1
    CASE_OF(HTTP_QUERY_CONTENT_TRANSFER_ENCODING);  // 2
    CASE_OF(HTTP_QUERY_CONTENT_ID);                 // 3
    CASE_OF(HTTP_QUERY_CONTENT_DESCRIPTION);        // 4
    CASE_OF(HTTP_QUERY_CONTENT_LENGTH);             // 5
    CASE_OF(HTTP_QUERY_CONTENT_LANGUAGE);           // 6
    CASE_OF(HTTP_QUERY_ALLOW);                      // 7
    CASE_OF(HTTP_QUERY_PUBLIC);                     // 8
    CASE_OF(HTTP_QUERY_DATE);                       // 9
    CASE_OF(HTTP_QUERY_EXPIRES);                    // 10
    CASE_OF(HTTP_QUERY_LAST_MODIFIED);              // 11
    CASE_OF(HTTP_QUERY_MESSAGE_ID);                 // 12
    CASE_OF(HTTP_QUERY_URI);                        // 13
    CASE_OF(HTTP_QUERY_DERIVED_FROM);               // 14
    CASE_OF(HTTP_QUERY_COST);                       // 15
    CASE_OF(HTTP_QUERY_LINK);                       // 16
    CASE_OF(HTTP_QUERY_PRAGMA);                     // 17
    CASE_OF(HTTP_QUERY_VERSION);                    // 18
    CASE_OF(HTTP_QUERY_STATUS_CODE);                // 19
    CASE_OF(HTTP_QUERY_STATUS_TEXT);                // 20
    CASE_OF(HTTP_QUERY_RAW_HEADERS);                // 21
    CASE_OF(HTTP_QUERY_RAW_HEADERS_CRLF);           // 22
    CASE_OF(HTTP_QUERY_CONNECTION);                 // 23
    CASE_OF(HTTP_QUERY_ACCEPT);                     // 24
    CASE_OF(HTTP_QUERY_ACCEPT_CHARSET);             // 25
    CASE_OF(HTTP_QUERY_ACCEPT_ENCODING);            // 26
    CASE_OF(HTTP_QUERY_ACCEPT_LANGUAGE);            // 27
    CASE_OF(HTTP_QUERY_AUTHORIZATION);              // 28
    CASE_OF(HTTP_QUERY_CONTENT_ENCODING);           // 29
    CASE_OF(HTTP_QUERY_FORWARDED);                  // 30
    CASE_OF(HTTP_QUERY_FROM);                       // 31
    CASE_OF(HTTP_QUERY_IF_MODIFIED_SINCE);          // 32
    CASE_OF(HTTP_QUERY_LOCATION);                   // 33
    CASE_OF(HTTP_QUERY_ORIG_URI);                   // 34
    CASE_OF(HTTP_QUERY_REFERER);                    // 35
    CASE_OF(HTTP_QUERY_RETRY_AFTER);                // 36
    CASE_OF(HTTP_QUERY_SERVER);                     // 37
    CASE_OF(HTTP_QUERY_TITLE);                      // 38
    CASE_OF(HTTP_QUERY_USER_AGENT);                 // 39
    CASE_OF(HTTP_QUERY_WWW_AUTHENTICATE);           // 40
    CASE_OF(HTTP_QUERY_PROXY_AUTHENTICATE);         // 41
    CASE_OF(HTTP_QUERY_ACCEPT_RANGES);              // 42
    CASE_OF(HTTP_QUERY_SET_COOKIE);                 // 43
    CASE_OF(HTTP_QUERY_COOKIE);                     // 44
    CASE_OF(HTTP_QUERY_REQUEST_METHOD);             // 45
    CASE_OF(HTTP_QUERY_REFRESH);                    // 46
    CASE_OF(HTTP_QUERY_CONTENT_DISPOSITION);        // 47
    CASE_OF(HTTP_QUERY_AGE);                        // 48
    CASE_OF(HTTP_QUERY_CACHE_CONTROL);              // 49
    CASE_OF(HTTP_QUERY_CONTENT_BASE);               // 50
    CASE_OF(HTTP_QUERY_CONTENT_LOCATION);           // 51
    CASE_OF(HTTP_QUERY_CONTENT_MD5);                // 52
    CASE_OF(HTTP_QUERY_CONTENT_RANGE);              // 53
    CASE_OF(HTTP_QUERY_ETAG);                       // 54
    CASE_OF(HTTP_QUERY_HOST);                       // 55
    CASE_OF(HTTP_QUERY_IF_MATCH);                   // 56
    CASE_OF(HTTP_QUERY_IF_NONE_MATCH);              // 57
    CASE_OF(HTTP_QUERY_IF_RANGE);                   // 58
    CASE_OF(HTTP_QUERY_IF_UNMODIFIED_SINCE);        // 59
    CASE_OF(HTTP_QUERY_MAX_FORWARDS);               // 60
    CASE_OF(HTTP_QUERY_PROXY_AUTHORIZATION);        // 61
    CASE_OF(HTTP_QUERY_RANGE);                      // 62
    CASE_OF(HTTP_QUERY_TRANSFER_ENCODING);          // 63
    CASE_OF(HTTP_QUERY_UPGRADE);                    // 64
    CASE_OF(HTTP_QUERY_VARY);                       // 65
    CASE_OF(HTTP_QUERY_VIA);                        // 66
    CASE_OF(HTTP_QUERY_WARNING);                    // 67
    CASE_OF(HTTP_QUERY_EXPECT);                     // 68
    CASE_OF(HTTP_QUERY_PROXY_CONNECTION);           // 69
    CASE_OF(HTTP_QUERY_UNLESS_MODIFIED_SINCE);      // 70
    CASE_OF(HTTP_QUERY_ECHO_REQUEST);               // 71
    CASE_OF(HTTP_QUERY_ECHO_REPLY);                 // 72
    CASE_OF(HTTP_QUERY_ECHO_HEADERS);               // 73
    CASE_OF(HTTP_QUERY_ECHO_HEADERS_CRLF);          // 74
    CASE_OF(HTTP_QUERY_CUSTOM);                     // 65535
    }
    return "?";
}


LPSTR
InternetMapHttpState(
    IN DWORD State
    )

/*++

Routine Description:

    Convert HTTPREQ_STATE_ to symbolic name

Arguments:

    State   - to map

Return Value:

    LPSTR

--*/

{
    switch (State) {
    CASE_OF(HttpRequestStateCreating);
    CASE_OF(HttpRequestStateOpen);
    CASE_OF(HttpRequestStateRequest);
    CASE_OF(HttpRequestStateResponse);
    CASE_OF(HttpRequestStateObjectData);
    CASE_OF(HttpRequestStateError);
    CASE_OF(HttpRequestStateClosing);
    CASE_OF(HttpRequestStateReopen);
    }
    return "?";
}

LPSTR
InternetMapHttpStateFlag(
    IN DWORD Flag
    )

/*++

Routine Description:

    Convert HTTPREQ_STATE_ flag to symbolic name

Arguments:

    Flag    - to map

Return Value:

    LPSTR

--*/

{
    switch (Flag) {
    case HTTPREQ_STATE_ANYTHING_OK:
        return "ANYTHING";

    case HTTPREQ_STATE_CLOSE_OK:
        return "CLOSE";

    case HTTPREQ_STATE_ADD_OK:
        return "ADD";

    case HTTPREQ_STATE_SEND_OK:
        return "SEND";

    case HTTPREQ_STATE_READ_OK:
        return "READ";

    case HTTPREQ_STATE_QUERY_REQUEST_OK:
        return "QUERY_REQUEST";

    case HTTPREQ_STATE_QUERY_RESPONSE_OK:
        return "QUERY_RESPONSE";

    case HTTPREQ_STATE_REUSE_OK:
        return "REUSE";
    }
    return "?";
}


LPSTR
InternetMapAsyncRequest(
    IN AR_TYPE Type
    )

/*++

Routine Description:

    Convert AR_TYPE to symbolic name

Arguments:

    Type    - Async request type

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Type) {
    CASE_OF(AR_INTERNET_CONNECT);
    CASE_OF(AR_INTERNET_OPEN_URL);
    CASE_OF(AR_INTERNET_READ_FILE);
    CASE_OF(AR_INTERNET_WRITE_FILE);
    CASE_OF(AR_INTERNET_QUERY_DATA_AVAILABLE);
    CASE_OF(AR_INTERNET_FIND_NEXT_FILE);
    CASE_OF(AR_FTP_FIND_FIRST_FILE);
    CASE_OF(AR_FTP_GET_FILE);
    CASE_OF(AR_FTP_PUT_FILE);
    CASE_OF(AR_FTP_DELETE_FILE);
    CASE_OF(AR_FTP_RENAME_FILE);
    CASE_OF(AR_FTP_OPEN_FILE);
    CASE_OF(AR_FTP_CREATE_DIRECTORY);
    CASE_OF(AR_FTP_REMOVE_DIRECTORY);
    CASE_OF(AR_FTP_SET_CURRENT_DIRECTORY);
    CASE_OF(AR_FTP_GET_CURRENT_DIRECTORY);
    CASE_OF(AR_GOPHER_FIND_FIRST_FILE);
    CASE_OF(AR_GOPHER_OPEN_FILE);
    CASE_OF(AR_GOPHER_GET_ATTRIBUTE);
    CASE_OF(AR_HTTP_SEND_REQUEST);
    CASE_OF(AR_HTTP_BEGIN_SEND_REQUEST);
    CASE_OF(AR_HTTP_END_SEND_REQUEST);
    CASE_OF(AR_READ_PREFETCH);
    CASE_OF(AR_SYNC_EVENT);
    CASE_OF(AR_TIMER_EVENT);
    CASE_OF(AR_HTTP_REQUEST1);
    CASE_OF(AR_FILE_IO);
    CASE_OF(AR_INTERNET_READ_FILE_EX);
    }
    return "?";
}


LPSTR
InternetMapHandleType(
    IN DWORD HandleType
    )

/*++

Routine Description:

    Map handle type to symbolic name

Arguments:

    HandleType  - from handle object

Return Value:

    LPSTR

--*/

{
    switch (HandleType) {
    CASE_OF(TypeGenericHandle);
    CASE_OF(TypeInternetHandle);
    CASE_OF(TypeFtpConnectHandle);
    CASE_OF(TypeFtpFindHandle);
    CASE_OF(TypeFtpFindHandleHtml);
    CASE_OF(TypeFtpFileHandle);
    CASE_OF(TypeFtpFileHandleHtml);
    CASE_OF(TypeGopherConnectHandle);
    CASE_OF(TypeGopherFindHandle);
    CASE_OF(TypeGopherFindHandleHtml);
    CASE_OF(TypeGopherFileHandle);
    CASE_OF(TypeGopherFileHandleHtml);
    CASE_OF(TypeHttpConnectHandle);
    CASE_OF(TypeHttpRequestHandle);
    CASE_OF(TypeFileRequestHandle);
    CASE_OF(TypeWildHandle);
    }
    return "?";
}


LPSTR
InternetMapScheme(
    IN INTERNET_SCHEME Scheme
    )

/*++

Routine Description:

    Maps INTERNET_SCHEME_ to symbolic name

Arguments:

    Scheme  - to map

Return Value:

    LPSTR

--*/

{
    switch (Scheme) {
    CASE_OF(INTERNET_SCHEME_UNKNOWN);
    CASE_OF(INTERNET_SCHEME_DEFAULT);
    CASE_OF(INTERNET_SCHEME_FTP);
    CASE_OF(INTERNET_SCHEME_GOPHER);
    CASE_OF(INTERNET_SCHEME_HTTP);
    CASE_OF(INTERNET_SCHEME_HTTPS);
    }
    return "?";
}


LPSTR
InternetMapOpenType(
    IN DWORD OpenType
    )

/*++

Routine Description:

    Maps INTERNET_OPEN_TYPE_ to symbolic name

Arguments:

    OpenType    - to map

Return Value:

    LPSTR

--*/

{
    switch (OpenType) {
    CASE_OF(INTERNET_OPEN_TYPE_DIRECT);
    CASE_OF(INTERNET_OPEN_TYPE_PROXY);
    CASE_OF(INTERNET_OPEN_TYPE_PRECONFIG);
    }
    return "?";
}


LPSTR
InternetMapService(
    IN DWORD Service
    )

/*++

Routine Description:

    Maps INTERNET_SERVICE_ to symbolic name

Arguments:

    Service - to map

Return Value:

    LPSTR

--*/

{
    switch (Service) {
#if 0
    CASE_OF(INTERNET_SERVICE_URL);
#endif
    CASE_OF(INTERNET_SERVICE_FTP);
    CASE_OF(INTERNET_SERVICE_GOPHER);
    CASE_OF(INTERNET_SERVICE_HTTP);
    }
    return "?";
}


LPSTR
InternetMapWinsockCallbackType(
    IN DWORD CallbackType
    )

/*++

Routine Description:

    Maps WINSOCK_CALLBACK_ to symbolic name

Arguments:

    Service - to map

Return Value:

    LPSTR

--*/

{
    switch (CallbackType) {
    CASE_OF(WINSOCK_CALLBACK_CONNECT);
    CASE_OF(WINSOCK_CALLBACK_GETHOSTBYADDR);
    CASE_OF(WINSOCK_CALLBACK_GETHOSTBYNAME);
    CASE_OF(WINSOCK_CALLBACK_LISTEN);
    CASE_OF(WINSOCK_CALLBACK_RECVFROM);
    CASE_OF(WINSOCK_CALLBACK_SENDTO);
    }
    return "?";
}

//
// private functions
//


PRIVATE
LPSTR
ExtractFileName(
    IN LPSTR Module,
    OUT LPSTR Buf
    )
{
    LPSTR filename;
    LPSTR extension;
    int   len;

    filename = strrchr(Module, '\\');
    extension = strrchr(Module, '.');
    if (filename) {
        ++filename;
    } else {
        filename = Module;
    }
    if (!extension) {
        extension = filename + strlen(filename);
    }
    len = (int) (extension - filename);
    memcpy(Buf, filename, len);
    Buf[len] = '\0';
    return Buf;
}


PRIVATE
LPSTR
SetDebugPrefix(
    IN LPSTR Buffer
    )
{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    //INET_ASSERT(lpThreadInfo != NULL);

    if (!lpThreadInfo) {
        return NULL;
    }

    LPINTERNET_DEBUG_RECORD lpRecord;

    lpRecord = lpThreadInfo->Stack;

    if (InternetDebugControlFlags & DBG_ENTRY_TIME) {
        if ((InternetDebugControlFlags & (DBG_DELTA_TIME | DBG_CUMULATIVE_TIME))
        && lpRecord) {

            DWORD ticks;
            DWORD ticksNow;

            ticksNow = GetTickCount();
            ticks = ticksNow
                  - ((InternetDebugControlFlags & DBG_CUMULATIVE_TIME)
                        ? InternetDebugStartTime
                        : lpRecord->LastTime
                        );

            Buffer += rsprintf(Buffer,
                               "% 5d.%3d ",
                               ticks / 1000,
                               ticks % 1000
                               );
            if (InternetDebugControlFlags & DBG_DELTA_TIME) {
                lpRecord->LastTime = ticksNow;
            }
        } else {

            SYSTEMTIME timeNow;

            InternetDebugGetLocalTime(&timeNow, NULL);

            Buffer += rsprintf(Buffer,
                               "%02d:%02d:%02d.%03d ",
                               timeNow.wHour,
                               timeNow.wMinute,
                               timeNow.wSecond,
                               timeNow.wMilliseconds
                               );
        }
    }

/*
    if (InternetDebugControlFlags & DBG_LEVEL_INDICATOR) {
        Buffer += rsprintf(Buffer, );
    }
*/

    if (InternetDebugControlFlags & DBG_THREAD_INFO) {

        //
        // thread id
        //

        Buffer += rsprintf(Buffer, "%08x", lpThreadInfo->ThreadId);

        //
        // INTERNET_THREAD_INFO address
        //

        if (InternetDebugControlFlags & DBG_THREAD_INFO_ADR) {
            Buffer += rsprintf(Buffer, ":%08x", lpThreadInfo);
        }

        //
        // ARB address
        //

        //if (InternetDebugControlFlags & DBG_ARB_ADDR) {
        //    Buffer += rsprintf(Buffer, ":%08x", lpThreadInfo->lpArb);
        //}

        //
        // FIBER address
        //

        //if (InternetDebugControlFlags & DBG_FIBER_INFO) {
        //
        //    LPVOID p;
        //
        //    p = (lpThreadInfo->lpArb != NULL)
        //      ? lpThreadInfo->lpArb->Header.lpFiber
        //      : NULL
        //      ;
        //    Buffer += rsprintf(Buffer, ":%08x", p);
        //}

        //
        // async ID
        //

        if (InternetDebugControlFlags & DBG_ASYNC_ID) {
            if (lpThreadInfo->IsAsyncWorkerThread) {
                Buffer += rsprintf(Buffer, ":<--->");
            } else if (lpThreadInfo->InCallback) {

                //
                // async worker thread calling back into the app; any WinInet
                // API requests during this time treated as though from the
                // app context
                //

                Buffer += rsprintf(Buffer, ":<c-b>");
            }
            else if (lpThreadInfo->IsAutoProxyProxyThread)
            {

                //
                // this is a specialized Auto-Proxy thread
                //

                Buffer += rsprintf(Buffer, ":<a-p>");
            }
            else
            {

                //
                // this is an app thread
                //

                Buffer += rsprintf(Buffer, ":<app>");
            }
        }

        //
        // request handle
        //

        if (InternetDebugControlFlags & DBG_REQUEST_HANDLE) {
            Buffer += rsprintf(Buffer, ":%6X", lpThreadInfo->hObject);
        }

        *Buffer++ = ' ';
    }

    if (InternetDebugControlFlags & DBG_CALL_DEPTH) {
        Buffer += rsprintf(Buffer, "%03d ", lpThreadInfo->CallDepth);
    }

    for (int i = 0; i < lpThreadInfo->IndentIncrement; ++i) {
        *Buffer++ = ' ';
    }

    //
    // if we are not debugging the category - i.e we got here via a requirement
    // to display an error, or we are in a function that does not have a
    // DEBUG_ENTER - then prefix the string with the current function name
    // (obviously misleading if the function doesn't have a DEBUG_ENTER)
    //

    if (lpRecord != NULL) {
        if (!(lpRecord->Category & InternetDebugCategoryFlags)) {
            Buffer += rsprintf(Buffer, "%s(): ", lpRecord->Function);
        }
    }

    return Buffer;
}

int dprintf(char * format, ...) {

    va_list args;
    char buf[PRINTF_STACK_BUFFER_LENGTH];
    int n;

    va_start(args, format);
    n = _sprintf(buf, format, args);
    va_end(args);
    OutputDebugString(buf);

    return n;
}


LPSTR
SourceFilename(
    LPSTR Filespec
    )
{
    if (!Filespec) {
        return "?";
    }

    LPSTR p;

    if (p = strrchr(Filespec, '\\')) {

        //
        // we want e.g. common\debugmem.cxx, but get
        // common\..\win32\debugmem.cxx. Bah!
        //

        //LPSTR q;
        //
        //if (q = strrchr(p - 1, '\\')) {
        //    p = q;
        //}
    }
    return p ? p + 1 : Filespec;
}

typedef BOOL (* SYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL (* SYMLOADMODULE)(HANDLE, HANDLE, PSTR, PSTR, DWORD, DWORD);
typedef BOOL (* SYMGETSYMFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (* SYMCLEANUP)(HANDLE);

PRIVATE HMODULE hSymLib = NULL;
PRIVATE SYMINITIALIZE pSymInitialize = NULL;
PRIVATE SYMLOADMODULE pSymLoadModule = NULL;
PRIVATE SYMGETSYMFROMADDR pSymGetSymFromAddr = NULL;
PRIVATE SYMCLEANUP pSymCleanup = NULL;


VOID
InitSymLib(
    VOID
    )
{
    if (hSymLib == NULL) {
        hSymLib = LoadLibrary("IMAGEHLP.DLL");
        if (hSymLib != NULL) {
            pSymInitialize = (SYMINITIALIZE)GetProcAddress(hSymLib,
                                                           "SymInitialize"
                                                           );
            pSymLoadModule = (SYMLOADMODULE)GetProcAddress(hSymLib,
                                                           "SymLoadModule"
                                                           );
            pSymGetSymFromAddr = (SYMGETSYMFROMADDR)GetProcAddress(hSymLib,
                                                                   "SymGetSymFromAddr"
                                                                   );
            pSymCleanup = (SYMCLEANUP)GetProcAddress(hSymLib,
                                                     "SymCleanup"
                                                     );
            if (!pSymInitialize
            || !pSymLoadModule
            || !pSymGetSymFromAddr
            || !pSymCleanup) {
                FreeLibrary(hSymLib);
                hSymLib = NULL;
                pSymInitialize = NULL;
                pSymLoadModule = NULL;
                pSymGetSymFromAddr = NULL;
                pSymCleanup = NULL;
                return;
            }
        }
        pSymInitialize(GetCurrentProcess(), NULL, FALSE);
        //SymInitialize(GetCurrentProcess(), NULL, TRUE);
        pSymLoadModule(GetCurrentProcess(), NULL, "WININET.DLL", "WININET", 0, 0);
    }
}


VOID
TermSymLib(
    VOID
    )
{
    if (pSymCleanup) {
        pSymCleanup(GetCurrentProcess());
        FreeLibrary(hSymLib);
    }
}


LPSTR
GetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    )
{
    *Offset = Address;
    if (!pSymGetSymFromAddr) {
        return "";
    }

    //
    // BUGBUG - only one caller at a time please
    //

    static char symBuf[512];

    //((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    ((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(symBuf);
    ((PIMAGEHLP_SYMBOL)symBuf)->MaxNameLength = sizeof(symBuf) - sizeof(IMAGEHLP_SYMBOL);
    if (!pSymGetSymFromAddr(GetCurrentProcess(),
                            Address,
                            Offset,
                            (PIMAGEHLP_SYMBOL)symBuf)) {
        ((PIMAGEHLP_SYMBOL)symBuf)->Name[0] = '\0';
    }
    return ((PIMAGEHLP_SYMBOL)symBuf)->Name;
}

#if defined(i386)


VOID
x86SleazeCallStack(
    OUT LPVOID * lplpvStack,
    IN DWORD dwStackCount,
    IN LPVOID * Ebp
    )

/*++

Routine Description:

    Similar to x86SleazeCallersAddress but gathers a variable number of return
    addresses. We assume all functions have stack frame

Arguments:

    lplpvStack      - pointer to returned array of caller's addresses

    dwStackCount    - number of elements in lplpvStack

    Ebp             - starting Ebp if not 0, else use current stack

Return Value:

    None.

--*/

{
    DWORD my_esp;

    _asm mov my_esp, esp;

    __try {
        if (Ebp == 0) {
            Ebp = (LPVOID *)(&lplpvStack - 2);
        }
        while (dwStackCount--) {
            if (((DWORD)Ebp > my_esp + 0x10000) || ((DWORD)Ebp < my_esp - 0x10000)) {
                break;
            }
            *lplpvStack++ = *(Ebp + 1);
            Ebp = (LPVOID *)*Ebp;
            if (((DWORD)Ebp <= 0x10000)
            || ((DWORD)Ebp >= 0x80000000)
            || ((DWORD)Ebp & 3)
            || ((DWORD)Ebp > my_esp + 0x10000)
            || ((DWORD)Ebp < my_esp - 0x10000)) {
                break;
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
}


VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    )

/*++

Routine Description:

    This is a sleazy function that reads return addresses out of the stack/
    frame pointer (ebp). We pluck out the return address of the function
    that called THE FUNCTION THAT CALLED THIS FUNCTION, and the caller of
    that function. Returning the return address of the function that called
    this function is not interesting to that caller (almost worthy of Sir
    Humphrey Appleby isn't it?)

    Assumes:

        my ebp  =>  | caller's ebp |
                    | caller's eip |
                    | arg #1       | (pCaller)
                    | arg #2       | (pCallersCaller

Arguments:

    pCaller         - place where we return addres of function that called
                      the function that called this function
    pCallersCaller  - place where we return caller of above

Return Value:

    None.

--*/

{

    //
    // this only works on x86 and only if not fpo functions!
    //

    LPVOID* ebp;

    ebp = (PVOID*)&pCaller - 2; // told you it was sleazy
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCaller = *(ebp + 1);
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCallersCaller = *(ebp + 1);
}

#endif // defined(i386)

#endif // ENABLE_DEBUG

INTERNETAPI
BOOL
WINAPI
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwMicroSec
)
{
#ifndef ENABLE_DEBUG
    // QUICK HACK TO KEEP THINGS CLEAN AND STILL MEASURE WITH HIGH PERFORMANCE
    // COUNTER

    static BOOL pcTested = FALSE;
    static LONGLONG ftInit;  // initial local time
    static LONGLONG pcInit;  // initial perf counter
    static LONGLONG pcFreq;  // perf counter frequency

    if (!pcTested)
    {
        pcTested = TRUE;
        if (QueryPerformanceFrequency ((LARGE_INTEGER *) &pcFreq) && pcFreq)
        {
            QueryPerformanceCounter ((LARGE_INTEGER *) &pcInit);
            SYSTEMTIME st;
            GetLocalTime (&st);
            SystemTimeToFileTime (&st, (FILETIME *) &ftInit);
        }
    }

    if (!pcFreq)
        GetLocalTime (pstLocalTime);
    else
    {
        LONGLONG pcCurrent, ftCurrent;
        QueryPerformanceCounter ((LARGE_INTEGER *) &pcCurrent);
        ftCurrent = ftInit + ((10000000 * (pcCurrent - pcInit)) / pcFreq);
        FileTimeToSystemTime ((FILETIME *) &ftCurrent, pstLocalTime);
    }

    return TRUE;
#else
    if (!pcFreq)
        GetLocalTime (pstLocalTime);
    else
    {
        LONGLONG pcCurrent, ftCurrent;
        QueryPerformanceCounter ((LARGE_INTEGER *) &pcCurrent);
        ftCurrent = ftInit + ((10000000 * (pcCurrent - pcInit)) / pcFreq);
        FileTimeToSystemTime ((FILETIME *) &ftCurrent, pstLocalTime);
    }

    return TRUE;
#endif // ENABLE_DEBUG
}
