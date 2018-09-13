/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    filelog.c

Abstract:

    Routines for logging files in copy logs.

Author:

    Ted Miller (tedm) 14-Jun-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Define name of system log file and various strings used
// within it.
//
PCTSTR SystemLogFileName = TEXT("repair\\setup.log");
PCTSTR NtFileSectionName = TEXT("Files.WinNT");

//
// Define structure used internally to represent a file log file.
//
typedef struct _SETUP_FILE_LOG {
    PCTSTR FileName;
    BOOL QueryOnly;
    BOOL SystemLog;
} SETUP_FILE_LOG, *PSETUP_FILE_LOG;


#ifdef UNICODE
//
// ANSI version
//
HSPFILELOG
SetupInitializeFileLogA(
    IN PCSTR LogFileName,   OPTIONAL
    IN DWORD Flags
    )
{
    PWSTR p;
    DWORD d;
    HSPFILELOG h;

    if(LogFileName) {
        d = CaptureAndConvertAnsiArg(LogFileName,&p);
        if(d != NO_ERROR) {
            SetLastError(d);
            return(INVALID_HANDLE_VALUE);
        }
    } else {
        p = NULL;
    }

    h = SetupInitializeFileLogW(p,Flags);
    d = GetLastError();

    if(p) {
        MyFree(p);
    }

    SetLastError(d);
    return(h);
}
#else
//
// Unicode stub
//
HSPFILELOG
SetupInitializeFileLogW(
    IN PCWSTR LogFileName,  OPTIONAL
    IN DWORD  Flags
    )
{
    UNREFERENCED_PARAMETER(LogFileName);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif

HSPFILELOG
SetupInitializeFileLog(
    IN PCTSTR LogFileName,  OPTIONAL
    IN DWORD  Flags
    )

/*++

Routine Description:

    Initialize a file for logging or query. The caller may specify that he
    wishes to use the system log, which is where the system tracks which
    files are installed as part of Windows NT; or the caller may specify
    any other random file to be used as a log.

    If the user specifies the system log not for query only, the function fails
    unless the user is administrator. However this only guarantees security
    on the log when the system is installed on a drive with a filesystem that
    supports ACLs; the log is simply a file and anyone can access it unless
    setup can secure it via ACLs.

Arguments:

    LogFileName - if specified, supplies the filename of the file to be used
        as the log file. Must be specified if Flags does not include
        SPFILELOG_SYSTEMLOG. Must not be specified if Flags includes
        SPFILELOG_SYSTEMLOG.

    Flags - supplies a combination of the following values:

        SPFILELOG_SYSTEMLOG - use the Windows NT system file log, which is used
            to track what files are installed as part of Windows NT. The user must
            be administrator to specify this option unless SPFILELOG_QUERYONLY
            is specified, and LogFileName must not be specified. May not be specified
            in combination with SPFILELOG_FORCENEW.

        SPFILELOG_FORCENEW - if the log file exists, it will be overwritten.
            If the log file exists and this flag is not specified then additional
            files are added to the existing log. May not be specified in combination
            with SPFILELOG_SYSTEMLOG.

        SPFILELOG_QUERYONLY - open the log file for querying only. The user

Return Value:

    Handle to file log or INVALID_HANDLE_VALUE if the function fails;
    extended error info is available via GetLastError() in this case.

--*/

{
    TCHAR SysLogFileName[MAX_PATH];
    PCTSTR FileName;
    PSETUP_FILE_LOG FileLog;
    DWORD Err;
    HANDLE hFile;

    //
    // Validate args.
    //
    Err = ERROR_INVALID_PARAMETER;
    if(Flags & SPFILELOG_SYSTEMLOG) {
        if((Flags & SPFILELOG_FORCENEW) || LogFileName) {
            goto clean0;
        }
        //
        // User must be administrator to gain write access to system log.
        //
        if(!(Flags & SPFILELOG_QUERYONLY) && !IsUserAdmin()) {
            Err = ERROR_ACCESS_DENIED;
            goto clean0;
        }

        //
        // uses real windows directory BUGBUG!!! (jamiehun) make sure this is correct for Hydra
        //
        lstrcpy(SysLogFileName,WindowsDirectory);
        ConcatenatePaths(SysLogFileName,SystemLogFileName,MAX_PATH,NULL);
        FileName = SysLogFileName;
    } else {
        if(LogFileName) {
            if(!lstrcpyn(SysLogFileName,LogFileName,MAX_PATH)) {
                //
                // lstrcpyn faulted, LogFileName must be bad
                //
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
            FileName = SysLogFileName;
        } else {
            goto clean0;
        }
    }

    //
    // Allocate a log file structure.
    //
    Err = ERROR_NOT_ENOUGH_MEMORY;
    if(FileLog = MyMalloc(sizeof(SETUP_FILE_LOG))) {
        FileLog->FileName = DuplicateString(FileName);
        if(!FileLog->FileName) {
            goto clean1;
        }
    } else {
        goto clean0;
    }

    FileLog->QueryOnly = ((Flags & SPFILELOG_QUERYONLY) != 0);
    FileLog->SystemLog = ((Flags & SPFILELOG_SYSTEMLOG) != 0);

    //
    // See if the file exists.
    //
    if(FileExists(FileName,NULL)) {

        //
        // If it's the system log, take ownership of the file.
        //
        if(FileLog->SystemLog) {
            Err = TakeOwnershipOfFile(FileName);
            if(Err != NO_ERROR) {
                goto clean2;
            }
        }

        //
        // Set attribute to normal. This ensures we can delete/open/create the file
        // as appropriate below.
        //
        if(!SetFileAttributes(FileName,FILE_ATTRIBUTE_NORMAL)) {
            Err = GetLastError();
            goto clean2;
        }

        //
        // Delete the file now if the caller specified the force_new flag.
        //
        if((Flags & SPFILELOG_FORCENEW) && !DeleteFile(FileName)) {
            Err = GetLastError();
            goto clean2;
        }
    }

    //
    // Make sure we can open/create the file by attempting to do that now.
    //
    hFile = CreateFile(
                FileName,
                GENERIC_READ | (FileLog->QueryOnly ? 0 : GENERIC_WRITE),
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,            // Open if exists, create if not
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if(hFile == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto clean2;
    }

    CloseHandle(hFile);
    return((HSPFILELOG)FileLog);

clean2:
    MyFree(FileLog->FileName);
clean1:
    MyFree(FileLog);
clean0:
    SetLastError(Err);
    return(INVALID_HANDLE_VALUE);
}


BOOL
SetupTerminateFileLog(
    IN HSPFILELOG FileLogHandle
    )

/*++

Routine Description:

    Releases resources associated with a file log.

Arguments:

    FileLogHandle - supplies the handle to the file log, as returned
        by SetupInitializeLogFile.

Return Value:

    Boolean value indicating outcome. If FALSE, the caller can use
    GetLastError() to get extended error info.

--*/

{
    PSETUP_FILE_LOG FileLog;
    DWORD Err;

    FileLog = (PSETUP_FILE_LOG)FileLogHandle;
    Err = NO_ERROR;

    try {
        MyFree(FileLog->FileName);
        MyFree(FileLog);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupLogFileA(
    IN HSPFILELOG FileLogHandle,
    IN PCSTR      LogSectionName,   OPTIONAL
    IN PCSTR      SourceFilename,
    IN PCSTR      TargetFilename,
    IN DWORD      Checksum,         OPTIONAL
    IN PCSTR      DiskTagfile,      OPTIONAL
    IN PCSTR      DiskDescription,  OPTIONAL
    IN PCSTR      OtherInfo,        OPTIONAL
    IN DWORD      Flags
    )
{
    PWSTR logsectionname = NULL;
    PWSTR sourcefilename = NULL;
    PWSTR targetfilename = NULL;
    PWSTR disktagfile = NULL;
    PWSTR diskdescription = NULL;
    PWSTR otherinfo = NULL;
    DWORD d;
    BOOL b;

    if(LogSectionName) {
        d = CaptureAndConvertAnsiArg(LogSectionName,&logsectionname);
    } else {
        d = NO_ERROR;
    }
    if(d == NO_ERROR) {
        d = CaptureAndConvertAnsiArg(SourceFilename,&sourcefilename);
    }
    if(d == NO_ERROR) {
        d = CaptureAndConvertAnsiArg(TargetFilename,&targetfilename);
    }
    if((d == NO_ERROR) && DiskTagfile) {
        d = CaptureAndConvertAnsiArg(DiskTagfile,&disktagfile);
    }
    if((d == NO_ERROR) && DiskDescription) {
        d = CaptureAndConvertAnsiArg(DiskDescription,&diskdescription);
    }
    if((d == NO_ERROR) && OtherInfo) {
        d = CaptureAndConvertAnsiArg(OtherInfo,&otherinfo);
    }

    if(d == NO_ERROR) {

        b = SetupLogFileW(
                FileLogHandle,
                logsectionname,
                sourcefilename,
                targetfilename,
                Checksum,
                disktagfile,
                diskdescription,
                otherinfo,
                Flags
                );

        d = GetLastError();

    } else {
        b = FALSE;
    }

    if(logsectionname) {
        MyFree(logsectionname);
    }
    if(sourcefilename) {
        MyFree(sourcefilename);
    }
    if(targetfilename) {
        MyFree(targetfilename);
    }
    if(disktagfile) {
        MyFree(disktagfile);
    }
    if(diskdescription) {
        MyFree(diskdescription);
    }
    if(otherinfo) {
        MyFree(otherinfo);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupLogFileW(
    IN HSPFILELOG FileLogHandle,
    IN PCWSTR     LogSectionName,   OPTIONAL
    IN PCWSTR     SourceFilename,
    IN PCWSTR     TargetFilename,
    IN DWORD      Checksum,         OPTIONAL
    IN PCWSTR     DiskTagfile,      OPTIONAL
    IN PCWSTR     DiskDescription,  OPTIONAL
    IN PCWSTR     OtherInfo,        OPTIONAL
    IN DWORD      Flags
    )
{
    UNREFERENCED_PARAMETER(FileLogHandle);
    UNREFERENCED_PARAMETER(LogSectionName);
    UNREFERENCED_PARAMETER(SourceFilename);
    UNREFERENCED_PARAMETER(TargetFilename);
    UNREFERENCED_PARAMETER(Checksum);
    UNREFERENCED_PARAMETER(DiskTagfile);
    UNREFERENCED_PARAMETER(DiskDescription);
    UNREFERENCED_PARAMETER(OtherInfo);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupLogFile(
    IN HSPFILELOG FileLogHandle,
    IN PCTSTR     LogSectionName,   OPTIONAL
    IN PCTSTR     SourceFilename,
    IN PCTSTR     TargetFilename,
    IN DWORD      Checksum,         OPTIONAL
    IN PCTSTR     DiskTagfile,      OPTIONAL
    IN PCTSTR     DiskDescription,  OPTIONAL
    IN PCTSTR     OtherInfo,        OPTIONAL
    IN DWORD      Flags
    )

/*++

Routine Description:

    Logs a file into a file log.

Arguments:

    FileLogHandle - supplies the handle to the file log, as returned
        by SetupInitializeLogFile(). The caller must not have passed
        SPFILELOG_QUERYONLY when the log file was opened/initialized.

    LogSectionName - required if SPFILELOG_SYSTEMLOG was not passed when
        the file log was opened/initialized; optional otherwise.
        Supplies the name for a logical grouping of files within the log.

    SourceFilename - supplies the name of the file as it exists on the
        source media from which it was installed. This name should be in
        whatever format is meaningful to the caller.

    TargetFilename - supplies the name of the file as it exists on the
        Target. This name should be in whatever format is meaningful to
        the caller.

    Checksum - supplies a 32-bit checksum value. Required for the system log.

    DiskTagfile - Gives the tagfile for the media from which the file
        was installed. Required for the system log if SPFILELOG_OEMFILE
        is specified. Ignored for the system log if SPFILELOG_OEMFILE is
        not specified.

    DiskDescription - Gives the human-readable description for the media
        from which the file was installed. Required for the system log if
        SPFILELOG_OEMFILE is specified. Ignored for the system log if
        SPFILELOG_OEMFILE is not specified.

    OtherInfo - supplies additional information to be associated with the
        file.

    Flags - may supply SPFILELOG_OEMFILE, which is meaningful only for
        the system log and indicates that the file is not an MS-supplied file.
        Can be used to convert an existing file's entry such as when an oem
        overwrites an MS-supplied system file.

Return Value:

    Boolean value indicating outcome. If FALSE, the caller can use
    GetLastError() to get extended error info.

--*/

{
    PSETUP_FILE_LOG FileLog;
    DWORD Err;
    BOOL b;
    TCHAR LineToWrite[512];
    TCHAR sourceFilename[MAX_PATH];
    PTSTR p,Directory;

    FileLog = (PSETUP_FILE_LOG)FileLogHandle;

    try {
        //
        // Validate params. Handle must be for non-queryonly.
        // If for the system log and oem file is specified,
        // caller must have passed disk tagfile and description.
        // There's really no way to validate the checksum because
        // 0 is a perfectly valid one.
        // If not the system log, caller must have passed a section name.
        //
        if(FileLog->QueryOnly
        || (  FileLog->SystemLog
            && (Flags & SPFILELOG_OEMFILE)
            && (!DiskTagfile || !DiskDescription))
        || (!FileLog->SystemLog && !LogSectionName))
        {
            Err = ERROR_INVALID_PARAMETER;

        } else {
            //
            // Use default section if not specified.
            //
            if(!LogSectionName) {
                MYASSERT(FileLog->SystemLog);
                LogSectionName = NtFileSectionName;
            }

            //
            // IF THIS LOGIC IS CHANGED BE SURE TO CHANGE
            // SetupQueryFileLog() AS WELL!
            //
            // Split up the source filename into filename and
            // directory if appropriate.
            //
            lstrcpyn(sourceFilename,SourceFilename,MAX_PATH);
            if(FileLog->SystemLog && (Flags & SPFILELOG_OEMFILE)) {
                if(p = _tcsrchr(sourceFilename,TEXT('\\'))) {
                    *p++ = 0;
                    Directory = p;
                } else {
                    Directory = TEXT("\\");
                }
            } else {
                Directory = TEXT("");
            }

            _sntprintf(
                LineToWrite,
                sizeof(LineToWrite)/sizeof(LineToWrite[0]),
                TEXT("%s,%x,%s,%s,\"%s\""),
                sourceFilename,
                Checksum,
                Directory,
                DiskTagfile ? DiskTagfile : TEXT(""),
                DiskDescription ? DiskDescription : TEXT("")
                );


            b = WritePrivateProfileString(
                    LogSectionName,
                    TargetFilename,
                    LineToWrite,
                    FileLog->FileName
                    );

            Err = b ? NO_ERROR : GetLastError();
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupRemoveFileLogEntryA(
    IN HSPFILELOG FileLogHandle,
    IN PCSTR      LogSectionName,   OPTIONAL
    IN PCSTR      TargetFilename    OPTIONAL
    )
{
    PWSTR logsectionname,targetfilename;
    DWORD d;
    BOOL b;

    if(LogSectionName) {
        d = CaptureAndConvertAnsiArg(LogSectionName,&logsectionname);
        if(d != NO_ERROR) {
            SetLastError(d);
            return(FALSE);
        }
    } else {
        logsectionname = NULL;
    }
    if(TargetFilename) {
        d = CaptureAndConvertAnsiArg(TargetFilename,&targetfilename);
        if(d != NO_ERROR) {
            if(logsectionname) {
                MyFree(logsectionname);
            }
            SetLastError(d);
            return(FALSE);
        }
    } else {
        targetfilename = NULL;
    }

    b = SetupRemoveFileLogEntryW(FileLogHandle,logsectionname,targetfilename);
    d = GetLastError();

    if(logsectionname) {
        MyFree(logsectionname);
    }
    if(targetfilename) {
        MyFree(targetfilename);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupRemoveFileLogEntryW(
    IN HSPFILELOG FileLogHandle,
    IN PCWSTR     LogSectionName,   OPTIONAL
    IN PCWSTR     TargetFilename    OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(FileLogHandle);
    UNREFERENCED_PARAMETER(LogSectionName);
    UNREFERENCED_PARAMETER(TargetFilename);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupRemoveFileLogEntry(
    IN HSPFILELOG FileLogHandle,
    IN PCTSTR     LogSectionName,   OPTIONAL
    IN PCTSTR     TargetFilename    OPTIONAL
    )

/*++

Routine Description:

    Removes an entry or section from a file log.

Arguments:

    FileLogHandle - supplies the handle to the file log, as returned
        by SetupInitializeLogFile(). The caller must not have passed
        SPFILELOG_QUERYONLY when the log file was opened/initialized.

    LogSectionName - Supplies the name for a logical grouping of files
        within the log. Required for non-system logs; optional for the
        system log.

    TargetFilename - supplies the name of the file as it exists on the
        Target. This name should be in whatever format is meaningful to
        the caller. If not specified, the entire section specified by
        LogSectionName is removed. Removing the main section for NT files
        is not allowed.

Return Value:

    Boolean value indicating outcome. If FALSE, the caller can use
    GetLastError() to get extended error info.

--*/

{
    DWORD Err;
    PSETUP_FILE_LOG FileLog;
    BOOL b;

    FileLog = (PSETUP_FILE_LOG)FileLogHandle;

    try {

        Err = NO_ERROR;
        if(FileLog->QueryOnly) {
            Err = ERROR_INVALID_PARAMETER;
        } else {
            if(!LogSectionName) {
                if(FileLog->SystemLog) {
                    LogSectionName = NtFileSectionName;
                } else {
                    Err = ERROR_INVALID_PARAMETER;
                }
            }
            //
            // Diallow removing the main nt files section.
            //
            if((Err == NO_ERROR)
            && FileLog->SystemLog
            && !TargetFilename
            && !lstrcmpi(LogSectionName,NtFileSectionName))
            {
                Err = ERROR_INVALID_PARAMETER;
            }
        }

        if(Err == NO_ERROR) {
            b = WritePrivateProfileString(LogSectionName,TargetFilename,NULL,FileLog->FileName);
            Err = b ? NO_ERROR : GetLastError();
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueryFileLogA(
    IN  HSPFILELOG       FileLogHandle,
    IN  PCSTR            LogSectionName,   OPTIONAL
    IN  PCSTR            TargetFilename,
    IN  SetupFileLogInfo DesiredInfo,
    OUT PSTR             DataOut,          OPTIONAL
    IN  DWORD            ReturnBufferSize,
    OUT PDWORD           RequiredSize      OPTIONAL
    )
{
    PWSTR logsectionname;
    PWSTR targetfilename;
    WCHAR unicodedata[2048];
    PSTR ansidata;
    DWORD requiredsize;
    DWORD d;
    BOOL b;

    d = CaptureAndConvertAnsiArg(TargetFilename,&targetfilename);
    if(d != NO_ERROR) {
        SetLastError(d);
        return(FALSE);
    }
    if(LogSectionName) {
        d = CaptureAndConvertAnsiArg(LogSectionName,&logsectionname);
        if(d != NO_ERROR) {
            MyFree(targetfilename);
            SetLastError(d);
            return(FALSE);
        }
    } else {
        logsectionname = NULL;
    }

    b = SetupQueryFileLogW(
            FileLogHandle,
            logsectionname,
            targetfilename,
            DesiredInfo,
            unicodedata,
            sizeof(unicodedata)/sizeof(WCHAR),
            &requiredsize
            );

    d = GetLastError();

    if(b) {
        d = NO_ERROR;

        if(ansidata = UnicodeToAnsi(unicodedata)) {

            requiredsize = lstrlenA(ansidata)+1;

            if(RequiredSize) {
                try {
                    *RequiredSize = requiredsize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    b = FALSE;
                    d = ERROR_INVALID_PARAMETER;
                }
            }

            if(b && DataOut) {
                if(ReturnBufferSize >= requiredsize) {
                    if(!lstrcpyA(DataOut,ansidata)) {
                        //
                        // lstrcpy faulted, ReturnBuffer must be invalid
                        //
                        d = ERROR_INVALID_PARAMETER;
                        b = FALSE;
                    }
                } else {
                    d = ERROR_INSUFFICIENT_BUFFER;
                    b = FALSE;
                }
            }

            MyFree(ansidata);
        } else {
            b = FALSE;
            d = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    MyFree(targetfilename);
    if(logsectionname) {
        MyFree(logsectionname);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueryFileLogW(
    IN  HSPFILELOG       FileLogHandle,
    IN  PCWSTR           LogSectionName,   OPTIONAL
    IN  PCWSTR           TargetFilename,
    IN  SetupFileLogInfo DesiredInfo,
    OUT PWSTR            DataOut,          OPTIONAL
    IN  DWORD            ReturnBufferSize,
    OUT PDWORD           RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(FileLogHandle);
    UNREFERENCED_PARAMETER(LogSectionName);
    UNREFERENCED_PARAMETER(TargetFilename);
    UNREFERENCED_PARAMETER(DesiredInfo);
    UNREFERENCED_PARAMETER(DataOut);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueryFileLog(
    IN  HSPFILELOG       FileLogHandle,
    IN  PCTSTR           LogSectionName,   OPTIONAL
    IN  PCTSTR           TargetFilename,
    IN  SetupFileLogInfo DesiredInfo,
    OUT PTSTR            DataOut,          OPTIONAL
    IN  DWORD            ReturnBufferSize,
    OUT PDWORD           RequiredSize      OPTIONAL
    )

/*++

Routine Description:

     Returns information from a setup file log.

Arguments:

    FileLogHandle - supplies handle to open file log, as returned by
        SetupInitializeFileLog().

    LogSectionName - required for non-system logs; if not specified
        for the system log a default is supplied. Supplies the name
        for a logical grouping within the log that is meaningful
        to the caller.

    TargetFilename - supplies name of file for which log information
        is desired.

    DesiredInfo - supplies an ordinal indicating what information
        is desired about the file.

    DataOut - If specified, points to a buffer that receives the
        requested information for the file. Note that not all info
        is provided for every file; an error is not returned if an entry
        for the file exists in the log but is empty.

    ReturnBufferSize - supplies size of the buffer (in chars) pointed to
        by DataOut. If the buffer is too small and DataOut is specified,
        no data is stored and the function returns FALSE. If DataOut is
        not specified this value is ignored.

    RequiredSize - receives the number of characters (including the
        terminating nul) required to hold the result.

Return Value:

    Boolean value indicating result. If FALSE, extended error info is
    available from GetLastError().

--*/

{
    DWORD Err;
    PSETUP_FILE_LOG FileLog;
    BOOL b;
    TCHAR ProfileValue[2*MAX_PATH];
    INT n;
    DWORD d;
    PTCHAR Field,End,Info;
    UINT InfoLength;
    BOOL Quoted;

    FileLog = (PSETUP_FILE_LOG)FileLogHandle;

    try {
        //
        // Validate arguments.
        // Section name must be supplied for non-system log.
        //
        if((!FileLog->SystemLog && !LogSectionName)
        || (DesiredInfo >= SetupFileLogMax) || !TargetFilename) {
            Err = ERROR_INVALID_PARAMETER;
        } else {

            if(!LogSectionName) {
                MYASSERT(FileLog->SystemLog);
                LogSectionName = NtFileSectionName;
            }

            //
            // Query the log file via profile API.
            //
            d = GetPrivateProfileString(
                    LogSectionName,
                    TargetFilename,
                    TEXT(""),
                    ProfileValue,
                    sizeof(ProfileValue)/sizeof(ProfileValue[0]),
                    FileLog->FileName
                    );

            if(d) {
                //
                // We want to retreive the Nth item in the value we just
                // retreived, where N is based on what the caller wants.
                // This routine assumes that the SetupFileLogInfo enum is
                // in the same order as items appear in a line in the log!
                //
                Field = ProfileValue;
                n = 0;

                nextfield:
                //
                // Find the end of the current field, which is
                // the first comma or space, or the end of the value.
                // Skip leading spaces.
                //
                while(*Field == TEXT(' ')) {
                    Field++;
                }
                End = Field;
                Quoted = FALSE;
                while(*End) {
                    if(*End == TEXT('\"')) {
                        Quoted = !Quoted;
                    } else {
                        if(!Quoted && ((*End == TEXT(' ')) || (*End == TEXT(',')))) {
                            //
                            // Got the end of the field.
                            //
                            break;
                        }
                    }
                    End++;
                }

                //
                // At this point, Field points to the start of the field
                // and End points at the character that terminated it.
                //
                if(n == DesiredInfo) {
                    Info = Field;
                    InfoLength = (UINT)(End-Field);
                } else {
                    //
                    // Skip trailing spaces and the comma, if any.
                    //
                    while(*End == ' ') {
                        End++;
                    }
                    if(*End == ',') {
                        //
                        // More fields exist.
                        //
                        Field = End+1;
                        n++;
                        goto nextfield;
                    } else {
                        //
                        // Item doesn't exist.
                        //
                        Info = TEXT("");
                        InfoLength = 0;
                    }
                }

                if(RequiredSize) {
                    *RequiredSize = InfoLength+1;
                }
                Err = NO_ERROR;
                if(DataOut) {
                    if(ReturnBufferSize > InfoLength) {
                        lstrcpyn(DataOut,Info,InfoLength+1);
                    } else {
                        Err = ERROR_INSUFFICIENT_BUFFER;
                    }
                }
            } else {
                Err = ERROR_FILE_NOT_FOUND;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}

