/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Diagnositc/debug routines for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 17-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#if ASSERTS_ON

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(MyDllModuleHandle,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s%s",
        LineNumber,
        FileName,
        Condition,
        (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ? "\r\n" : "\n\nCall DebugBreak()?"
        );

    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        OutputDebugStringA(Msg);
        i = IDYES;
    } else {
        i = MessageBoxA(
                NULL,
                Msg,
                p,
                MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                );
    }

    if(i == IDYES) {
        DebugBreak();
    }
}

#else

//
// Need something to satisfy the export in setupapi.def
//
VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(LineNumber);
    UNREFERENCED_PARAMETER(Condition);
}

#endif


#define DEFAULT_LOG_NAME TEXT("setupapi.log")

typedef struct _SP_LOGFILE {
    HANDLE FileHandle;
    SetupapiLogLevel LogLevel;
} SP_LOGFILE, *PSP_LOGFILE;


PVOID
CreateSetupapiLog(
    IN LPCTSTR          FileName,    OPTIONAL
    IN SetupapiLogLevel LogLevel,
    IN BOOL             Append
    )

/*++

Routine Description:

    This routine creates a debug log file. The caller can specify a location
    or the routine will create a file in a default location (setupapi.log
    in the directory specified by the TMP environment variable).

Arguments:

    FileName - supplies the Win32 pathname of the file. If not specified,
        a default name of setupapi.log will be used.

    LogLevel - supplies the maximum log level. Messages that are above this
        log level are not logged.

    Append - if TRUE, any existing log file is opened for append. Otherwise
        any existing log file is blown away.

Return Value:

    Handle to log file, or NULL if the file could not be created.

--*/

{
    TCHAR filename[MAX_PATH];
    DWORD d;
    PSP_LOGFILE LogFile;

    LogFile = MyMalloc(sizeof(SP_LOGFILE));
    if(!LogFile) {
        return(NULL);
    }
    LogFile->LogLevel = LogLevel;

    //
    // Figure out the name to use.
    //
    if(FileName) {
        lstrcpyn(filename,FileName,MAX_PATH);
    } else {
        d = ExpandEnvironmentStrings(TEXT("%TMP%\\") DEFAULT_LOG_NAME,filename,MAX_PATH);
        if(!d || (d >= MAX_PATH)) {
            lstrcpy(filename,TEXT("C:\\") DEFAULT_LOG_NAME);
        }
    }

    //
    // Create the file. If this fails, bail.
    //
    LogFile->FileHandle = CreateFile(
                            filename,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            Append ? OPEN_ALWAYS : CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

    if(LogFile->FileHandle == INVALID_HANDLE_VALUE) {
        MyFree(LogFile);
        return(NULL);
    }

    if(Append) {
        SetFilePointer(LogFile->FileHandle,0,NULL,FILE_END);
        WriteTextToSetupapiLog(LogFile,0,TEXT("\r\n\r\n"));
    }

    WriteToSetupapiLog(LogFile,0,MSG_LOG_STARTED);

    if(GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,NULL,NULL,filename,MAX_PATH)) {
        WriteTextToSetupapiLog(LogFile,0,filename);
        if(GetTimeFormat(LOCALE_USER_DEFAULT,TIME_NOSECONDS,NULL,NULL,filename,MAX_PATH)) {
            WriteTextToSetupapiLog(LogFile,0,TEXT(" "));
            WriteTextToSetupapiLog(LogFile,0,filename);
        }
        WriteTextToSetupapiLog(LogFile,0,TEXT("\r\n\r\n"));
    }

    return(LogFile);
}


VOID
CloseSetupapiLog(
    IN PVOID LogFile
    )
{
    if(LogFile) {
        CloseHandle(((PSP_LOGFILE)LogFile)->FileHandle);
        MyFree(LogFile);
    }
}


BOOL
WriteTextToSetupapiLog(
    IN PVOID            LogFile,
    IN SetupapiLogLevel Level,
    IN LPCTSTR          Text
    )

/*++

Routine Description:

    This routine writes some text to a log, if the log is open.
    Text is converted to ANSI before being written.

Arguments:

    LogFile - supplies a 'handle' to the log file to be written, as returned
        by a previous call to CreateSetupapiLog.

    Level - supplies the log level of the text. If this is greater than
        the user-selected logging level, no logging is performed.

    Text - supplies the (nul-terminated) text to be written. CR/LF are NOT
        automatically appended.

Return Value:

    The return value is a boolean indicating whether the text was
    successfully written to the log file.

    If logging is not enabled or the text needs to be ignored because of
    the logging level, TRUE is returned.

--*/

{
#ifdef UNICODE
    int UnicodeCharCount;
    LPSTR AnsiText;
#else
    LPCSTR AnsiText;
#endif
    int AnsiByteCount;
    BOOL b;
    DWORD d;

    if(!LogFile || (Level > ((PSP_LOGFILE)LogFile)->LogLevel)) {
        return(TRUE);
    }

#ifdef UNICODE
    UnicodeCharCount = lstrlen(Text);
    if(!UnicodeCharCount) {
        return(TRUE);
    }

    AnsiByteCount = WideCharToMultiByte(CP_ACP,0,Text,UnicodeCharCount,NULL,0,NULL,NULL);
    if(!AnsiByteCount) {
        return(FALSE);
    }

    AnsiText = LocalAlloc(LMEM_FIXED,AnsiByteCount);
    if(!AnsiText) {
        return(FALSE);
    }

    if(WideCharToMultiByte(CP_ACP,0,Text,UnicodeCharCount,AnsiText,AnsiByteCount,NULL,NULL) != AnsiByteCount) {
        LocalFree(AnsiText);
        return(FALSE);
    }
#else
    AnsiText = Text;
    AnsiByteCount = lstrlen(Text);
#endif

    b = WriteFile(((PSP_LOGFILE)LogFile)->FileHandle,AnsiText,AnsiByteCount,&d,NULL);

#ifdef UNICODE
    LocalFree(AnsiText);
#endif
    return(b);
}


BOOL
WriteToSetupapiLog(
    IN PVOID            LogFile,
    IN SetupapiLogLevel Level,
    IN UINT             MessageId,
    ...
    )

/*++

Routine Description:

    This routine writes an entry to a log, if the log is
    currently open. Text to be written comes from the message table.

Arguments:

    LogFile - supplies a 'handle' to the log file to be written, as returned
        by a previous call to CreateSetupapiLog.

    Level - supplies the log level of the text. If this is greater than
        the user-selected logging level, no logging is performed.

    MessageId - supplies the message id of the message table resource
        of the text for the logging.

    Additional arguments are message-dependent.

Return Value:

    Boolean value indicating whether the item was logged successfully.

    If logging is not enabled or the text needs to be ignored because of
    the logging level, TRUE is returned.

--*/

{
    DWORD d;
    LPTSTR text;
    va_list arglist;
    BOOL b;

    if(!LogFile || (Level > ((PSP_LOGFILE)LogFile)->LogLevel)) {
        return(TRUE);
    }

    va_start(arglist,MessageId);

    //
    // We don't use FormatMessageA since that implies ANSI substitutions,
    // and ours are Unicode.
    //
    text = NULL;
    d = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            MyDllModuleHandle,
            MessageId,
            0,
            (LPTSTR)&text,
            0,
            &arglist
            );

    va_end(arglist);

    if(!d) {
        if(text) {
            //
            // Strange case of intentionally empty message
            //
            LocalFree(text);
            return(TRUE);
        }
        return(FALSE);
    }

    b = WriteTextToSetupapiLog(LogFile,Level,text);

    LocalFree(text);
    return(b);
}
