/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cntxtlog.c

Abstract:

    This module implements more logging for setupapi

Author:

    Gabe Schaffer (t-gabes) 25-Jun-1998

Revision History:

    Jamie Hunter (jamiehun) Aug 31 1998

--*/

#include "precomp.h"
#pragma hdrstop

//
// BUGBUG!!! (jamiehun)
// _USE_LOCKING comments out potential improvement of code
// that isn't Win9x friendly.
//

// process-wide log counter
static LONG GlobalLogCount = 0;
static LONG GlobalUID = 0;

//
// This will compile always so that users of free builds can still have
// log output sent to the debugger.
// use MYTRACE for DBG enabled tracing
//
VOID
DebugPrint(
    PCTSTR format,
    ...                                 OPTIONAL
    )

/*++

Routine Description:

    Send a formatted string to the debugger.

Arguments:

    format - standard printf format string.

Return Value:

    NONE.

--*/

{
    TCHAR buf[1026];    // bigger than max size
    va_list arglist;

    va_start(arglist, format);
    wvsprintf(buf, format, arglist);

    OutputDebugString(buf);
}


__inline // we want to always optimize this out
BOOL
_WouldNeverLog(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD Level
    )

/*++

Routine Description:

    Determines if at the current logging level and the required level, we would never log
    inline'd for optimization (used only in this file)
    no locking performed - safe to call if locked, or if we know we won't decrement
    usage count below 1.


Arguments:

    LogContext - optionally supplies a pointer to the SETUP_LOG_CONTEXT to be
        used for logging. If not supplied always return FALSE.
    Level - only required to check for special case of 0.

Return Value:

    TRUE if we know we would never log based on passed information

--*/

{

    if (LogContext == NULL) {
        //
        // can never be sure
        //
        return FALSE;
    }
    if (Level == 0) {
        //
        // don't-log level
        //
        return TRUE;
    }

    if (((LogContext->LogLevel & SETUP_LOG_LEVELMASK) <= SETUP_LOG_NOLOG)
        &&((LogContext->LogLevel & DRIVER_LOG_LEVELMASK) <= DRIVER_LOG_NOLOG)) {
        //
        // LogContext indicates do no logging at all
        //
        return TRUE;
    }

    return FALSE;
}

__inline // we want to always optimize this out
BOOL
_WouldLog(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD Level
    )

/*++

Routine Description:

    Determines if at the current logging level and the required level, we would log
    inline'd for optimization (used only in this file)
    no locking performed - safe to call if locked, or if we know we won't decrement
    usage count below 1.

    Note that if _WouldNeverLog is TRUE, _WouldLog is always FALSE
    if _WouldLog is TRUE, _WouldNeverLog is always FALSE
    if both are FALSE, then we are on "maybe"

Arguments:

    LogContext - optionally supplies a pointer to the SETUP_LOG_CONTEXT to be
        used for logging. If not supplied always return FALSE.
    Level - bitmask indicating logging flags. See SETUP_LOG_* and DRIVER_LOG_*
        at the beginning of cntxtlog.h for details. It may also be a slot
        returned by AllocLogInfoSlot, or 0 (no logging)

Return Value:

    TRUE if we know we would log

--*/

{

    if (LogContext == NULL || _WouldNeverLog(LogContext,Level)) {
        //
        // some simple tests (LogLevel==NULL is a not sure case)
        //
        return FALSE;
    }

    if ((Level & SETUP_LOG_IS_CONTEXT)!=0) {
        //
        // context logging - ignored here (a not sure case)
        //
        return FALSE;
    }

    //
    // determine logability
    //
    if ((Level & SETUP_LOG_LEVELMASK) > 0 && (Level & SETUP_LOG_LEVELMASK) <= (LogContext->LogLevel & SETUP_LOG_LEVELMASK)) {
        //
        // we're interested in logging - raw error level
        //
        return TRUE;
    }
    if ((Level & DRIVER_LOG_LEVELMASK) > 0 && (Level & DRIVER_LOG_LEVELMASK) <= (LogContext->LogLevel & DRIVER_LOG_LEVELMASK)) {
        //
        // we're interested in logging - driver error level
        //
        return TRUE;
    }

    return FALSE;
}

VOID
UnMapLogFile(
    IN PSTR baseaddr,
    IN HANDLE hLogfile,
    IN HANDLE hMapping,
    IN DWORD locksize,
    IN BOOL seteof
    )

/*++

Routine Description:

    Unmap, possibly unlock, maybe set the EOF, and close a file.  Note, setting
    EOF must occur after unmapping.

Arguments:

    baseaddr - this is the address where the file is mapped.  It must be what
        was returned by MapLogFile.

    hLogfile - this is the Win32 handle for the log file.

    hMapping - this is the Win32 handle to the mapping object.

    locksize - if 0, or -1, indicates that no part of the file is locked, otherwise
        it indicates how many bytes are locked starting at the beginning of
        the file.  UnlockFile is called if locksize is valid.

    seteof - Boolean value indicating whether the EOF should be set to the
        current file pointer.  If the EOF is set and the file pointer has not
        been moved, the EOF will be set at byte 0, thus truncating the file
        to 0 bytes.

Return Value:

    NONE.

--*/

{
    DWORD success;

    //
    // we brute-force try to close everything up
    //

    try {
        if (baseaddr != NULL) {
            success = UnmapViewOfFile(baseaddr);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hMapping != NULL) {
            //
            // hMapping uses NULL to indicate a problem
            //
            success = CloseHandle(hMapping);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hLogfile != INVALID_HANDLE_VALUE && seteof) {
            success = SetEndOfFile(hLogfile);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hLogfile != INVALID_HANDLE_VALUE && locksize != (DWORD)(-1) && locksize != 0) {
    #ifdef _USE_LOCKING // we can't lock the file under Win9x
            success = UnlockFile(
                hLogfile,   // file handle
                0,          // offset low
                0,          // offset high
                locksize,   // size low
                0);         // size high
    #endif  // _USE_LOCKING
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }

    try {
        if (hLogfile != INVALID_HANDLE_VALUE) {
            success = CloseHandle(hLogfile);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing
        //
    }
    //
    // Win9x provides no way to wait for a file to become unlocked, so we
    // have to poll. Putting this Sleep(0) allows others to have a chance
    // at the file.
    //
    Sleep(0);
}

DWORD
MapLogFile(
    IN PCTSTR FileName,
    OUT PHANDLE hLogfile,
    OUT PHANDLE hMapping,
    OUT PDWORD dwLocksize,
    OUT PDWORD dwFilesize,
    OUT PSTR *mapaddr,
    IN DWORD extrabytes
    )

/*++

Routine Description:

    Open the log file for writing and memory map it.  On NT the file is locked,
    but Win9x doesn't allow memory mapped access to locked files, so the file
    is opened without FILE_SHARE_WRITE access.  Since CreateFile won't block
    like LockFileEx, we have to poll once per second on Win9x until the file
    opens.

Arguments:

    FileName - supplies path name to the log file.

    hLogfile - receives the Win32 file handle for the log file.

    hMapping - receives the Win32 handle to the mapping object.

    dwLockSize - receives the number of bytes (starting from the beginning of
        the file) that are locked.  This will be 0 if no part of the file is
        locked, which is different from not being shared.

    dwFileSize - receives the size of the file before it is mapped, because
        mapping increases the size of the file by extrabytes.

    mapaddr - receives the address of where the log file is mapped.

    extrabytes - supplies the number of extra bytes (beyond the size of the
        file) to add to the size of the mapping object to allow for appending
        the new log line and possibly a section header.

Return Value:

    NO_ERROR if the file is successfully opened and mapped.  The caller must
    call UnMapLogFile when finished with the file.

    Win32 error code if the file is not open.

--*/

{
    HANDLE logfile = INVALID_HANDLE_VALUE;
    HANDLE mapping = NULL;
    DWORD locksize = 0;
    DWORD filesize = 0;
    DWORD lockretrywait = 1;
    PSTR baseaddr = NULL;
    DWORD retval = ERROR_INVALID_PARAMETER;

#ifdef _USE_LOCKING
    DWORD sharemode = FILE_SHARE_READ
        | FILE_SHARE_WRITE | FILE_SHARE_DELETE; // only for NT
    BOOL result;
    OVERLAPPED lockinfo;
#else
    DWORD sharemode = FILE_SHARE_READ;
#endif


    //
    // wrap it all up in a nice big try/except, because you just never know
    //
    try {

        //
        // give initial "failed" values
        // this also validates the pointers
        //
        *hLogfile = logfile;
        *hMapping = mapping;
        *dwLocksize = locksize;
        *dwFilesize = filesize;
        *mapaddr = baseaddr;

        do {
            //
            // retry here, in case lock fails
            //
            logfile = CreateFile(
                FileName,
                GENERIC_READ | GENERIC_WRITE,   // access mode
                sharemode,
                NULL,                           // security
                OPEN_ALWAYS,                    // open, or create if not already there
                //FILE_FLAG_WRITE_THROUGH,        // flags - ensures that if machine crashes in the next operation, we are still logged
                0,
                NULL);                          // template

            if (logfile == INVALID_HANDLE_VALUE) {
                retval = GetLastError();
                if (retval != ERROR_SHARING_VIOLATION) {
                    MYTRACE(TEXT("Could not create file %s. Error %d\n"), FileName, retval);
                    leave;
                }
                //
                // don't want to wait more than a second
                //
                if (lockretrywait < 1000) {
                    lockretrywait *= 2;
                }
                MYTRACE(TEXT("Could not open file. Error %d; waiting %ums\n"), GetLastError(), lockretrywait);

                Sleep(lockretrywait);
            }
        } while (logfile == INVALID_HANDLE_VALUE);

#ifdef _USE_LOCKING
        //
        // this will NOT work with files >= 4GB, but it's not supposed to
        //
        locksize = GetFileSize(logfile,
            NULL);  // this is the high DWORD, just ignore it

        if (locksize == (DWORD)(-1)) {
            retval = GetLastError();
            MYTRACE("Could not get file size. Error %d\n", retval);
            leave;
        }
        locksize += extrabytes;

        lockinfo.Offset = 0;
        lockinfo.OffsetHigh = 0;
        lockinfo.hEvent = 0;

        result = LockFileEx(
            logfile,    // file handle
            LOCKFILE_EXCLUSIVE_LOCK,    // flags
            0,          // reserved
            locksize,   // size low
            0,          // size high
            &lockinfo); // overlapped structure

        if (result == FALSE) {
            retval = GetLastError();
            MYTRACE("Could not lock file. Error %d. Thread %u\n", retval, GetCurrentThreadId());
            locksize = 0;
        }
#endif  // _USE_LOCKING

        //
        // this will NOT work with files >= 4GB, but it's not supposed to
        //
        filesize = GetFileSize(logfile,
            NULL);  // this is the high DWORD, just ignore it

        //
        // make the mapping object with extra space to accomodate the new log entry
        //
        mapping = CreateFileMapping(
            logfile,            // file to map
            NULL,               // security
            PAGE_READWRITE,     // protection
            0,                  // maximum size high
            filesize + extrabytes,      // maximum size low
            NULL);              // name

        if (mapping != NULL) {
            //
            // NULL isn't a bug, CreateFileMapping returns this
            // to indicate error, instead of INVALID_HANDLE_VALUE
            //

            //
            // now we have a section object, so attach it to the log file
            //
            baseaddr = (PSTR) MapViewOfFile(
                mapping,                // file mapping object
                FILE_MAP_ALL_ACCESS,    // desired access
                0,                      // file offset high
                0,                      // file offset low
                0);                     // number of bytes to map (0 = whole file)
        }
        else {
            retval = GetLastError();
            MYTRACE(TEXT("Could not create mapping. Error %d\n"), retval);
            leave;
        }

        if (baseaddr == NULL) {
            //
            // either the mapping object couldn't be created or
            // the file couldn't be mapped
            //
            retval = GetLastError();
            MYTRACE(TEXT("Could not map file. Error %d\n"), retval);
            leave;
        }

        //
        // now put everything where the caller can see it, but make sure we clean
        // up first
        //
        *hLogfile = logfile;
        *hMapping = mapping;
        *dwLocksize = locksize;
        *dwFilesize = filesize;
        *mapaddr = baseaddr;

        retval = NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // something bad happened, probably an AV, so just dump everything
        // and return an error meaning "Attempt to access invalid address."
        //
    }

    if (retval != NO_ERROR) {
        //
        // an error occurred, cleanup what we need to
        //
        UnMapLogFile(baseaddr, logfile, mapping, locksize, FALSE);
    }

    return retval;
}

BOOL
IsSectionHeader(
    IN PCSTR Header,
    IN DWORD Size,
    IN PCSTR Beginning
    )

/*++

Routine Description:

    Determines whether a given string starts with a section header.  This is
    the routine that essentially defines what a valid section header is.

Arguments:

    Header - supplies a pointer to what may be the first character in a header.

    Size - supplies the length of the string passed in, which is NOT the size
        of the header.

    Beginning - supplies a pointer to the beginning of the file.

Return Value:

    BOOL indicating if Header points to a valid section header.

--*/

{
    //
    // assume a header looks like [foobar]\r\n
    //
    DWORD i;
    //
    // state holds the value we're looking for
    UINT state = '[';

    //
    // a section header must always be either at the start of a line or at
    // the beginning of a file
    //
    if (Header != Beginning && Header[-1] != '\n')
        return FALSE;

    for (i = 0; i < Size; i++) {
        switch (state) {
        case '[':
            if (Header[i] == '[') {
                state = ']';
            } else {
                return FALSE;
            }
            break;

        case ']':
            if (Header[i] == ']') {
                state = '\r';
            }
            break;

        case '\r':
            if (Header[i] == '\r') {
                state = '\n';
            //
            // allow for the case where a line has a linefeed, but no CR
            //
            } else if (Header[i] == '\n') {
                return TRUE;
#if 0
            } else if (Header[i] == ':') { // BUGBUG!!! (jamiehun) - warey of this!
                return TRUE;
#endif
            } else {
                return FALSE;
            }
            break;

        case '\n':
            if (Header[i] == '\n') {
                return TRUE;
            } else {
                return FALSE;
            }
            //
            // break; -- commented out to avoid unreachable code
            //
        default:
            MYTRACE(TEXT("Invalid state! (%d)\n"), state);
            MYASSERT(0);
        }
    }

    return FALSE;
}

BOOL
IsEqualSection(
    IN PCSTR Section1,
    IN DWORD Len1,
    IN PCSTR Section2,
    IN DWORD Len2
    )

/*++

Routine Description:

    Says whether two ANSI strings both start with the same section header.  One of
    the strings must be just a section header, while the other one may be
    anything, such as the entire log file.

Arguments:

    Section1 - supplies the address of the first string.

    Len1 - supplies the length of the first string.

    Section2 - supplies the address of the second string.

    Len2 - supplies the length of the second string.

Return Value:

    BOOL indicating if the longer string starts with the shorter string.

--*/

{
    //
    // maxlen is the maximum length that both strings could be, and still be
    // the same section name
    //
    DWORD maxlen = Len2;

    if (Len1 < Len2) {
        maxlen = Len1;
    }

    if (_strnicmp(Section1, Section2, maxlen) == 0) {
        //
        // they're the same (ignoring case)
        //
        return TRUE;
    }

    return FALSE;
}

DWORD
AppendLogEntryToSection(
    IN PCTSTR FileName,
    IN PCSTR Section,
    IN PCSTR Entry,
    IN BOOL SimpleAppend
    )

/*++

Routine Description:

    Opens the log file, finds the appropriate section, moves it to the end of
    the file, appends the new entry, and closes the file.

Arguments:

    FileName - supplies the path name of the log file.

    Section - supplies the ANSI name of the section to be logged to.

    Entry - supplies the ANSI string to be logged.

    SimpleAppend - specifies whether entries will simply be appended to the log
        file or appended to the section where they belong.

Return Value:

    NO_ERROR if the entry gets written to the log file.

    Win32 error or exception code if anything went wrong.

--*/

{
    DWORD retval = NO_ERROR;
    DWORD fpoff;
    HANDLE hLogfile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = NULL;
    DWORD locksize = 0;
    DWORD filesize = 0;
    PSTR baseaddr = NULL;
    DWORD sectlen = lstrlenA(Section);
    DWORD entrylen = lstrlenA(Entry);
    DWORD error;
    BOOL seteof = FALSE;
    BOOL mapped = FALSE;
    PSTR eof;
    PSTR curptr;
    PSTR lastsect = NULL;

    try {
        MYASSERT(Section != NULL && Entry != NULL);

        sectlen = lstrlenA(Section);
        entrylen = lstrlenA(Entry);
        if (sectlen == 0 || entrylen == 0) {
            //
            // not an error as such, but not useful either
            //
            retval = NO_ERROR;
            leave;
        }

        error = MapLogFile(
                    FileName,
                    &hLogfile,
                    &hMapping,
                    &locksize,
                    &filesize,
                    &baseaddr,
                    sectlen + entrylen + 8);// add some extra space to the mapping
                                            // to take into account the log entry
                                            // +2 to terminate unterminated last line
                                            // +2 to append CRLF or ": " after section
                                            // +2 to append CRLF after entrylen if req
                                            // +2 for good measure
        if (error != NO_ERROR) {
            //
            // could not map file
            //
            retval = error;
            leave;
        }

        mapped = TRUE;

        eof = baseaddr + filesize; // end of file, as of now
        curptr = eof;

        while (curptr > baseaddr && (curptr[-1]==0 || curptr[-1]==0x1A)) {
            //
            // eat up trailing Nul's or ^Z's
            // the former is a side-effect of mapping
            // the latter could be introduced by an editor
            //
            curptr --;
            eof = curptr;
        }
        if (eof > baseaddr && eof[-1] != '\n') {
            //
            // make sure file already ends in LF
            // if it doesn't, append a CRLF
            //
            memcpy(eof, "\r\n", 2);
            eof += 2;
        }
        if (SimpleAppend) {
            //
            // instead of having a regular section header, the section name is
            // placed at the beginning of each log line followed by a colon.
            // this is particularly only of interest when debugging the logging functions
            //
            memcpy(eof, Section, sectlen);
            eof += sectlen;
            memcpy(eof, ": ", 2);
            eof += 2;

        } else {
            //
            // the entry must be appended to the correct section in the log,
            // which requires finding the section and moving it to the end of
            // the file if required.
            //
            // search backwards in the file, looking for the section header
            //
            if (eof == baseaddr) {
                //
                // truncated (empty) file
                //
                curptr = NULL;
            } else {
                curptr = eof - 1;

                while(curptr > baseaddr) {
                    //
                    // scan for section header a line at a time
                    // going backwards, since our section should be near end
                    //
                    if (curptr[-1] == '\n') {
                        //
                        // speed optimization: only bother checking if we think we're at the beginning of a new line
                        // this may find a '\n' that is part of a MBCS char,
                        // but should be eliminated by IsSectionHeader check
                        //
                        if (IsSectionHeader(curptr, (DWORD)(eof - curptr), baseaddr)) {
                            //
                            // looks like a section header, now see if it's the one we want
                            //
                            if (IsEqualSection(curptr, (DWORD)(eof - curptr), Section, sectlen)) {
                                //
                                // yep - done
                                //
                                break;
                            } else {
                                //
                                // will eventually be the section after the one of interest
                                //
                                lastsect = curptr;
                            }
                        }
                    }
                    curptr --;
                }
                if (curptr == baseaddr) {
                    //
                    // final check if we got to the beginning of the file (no find)
                    //
                    if (IsSectionHeader(curptr, (DWORD)(eof - curptr), baseaddr)) {
                        //
                        // the first line should always be a section header
                        //
                        if (!IsEqualSection(curptr, (DWORD)(eof - curptr), Section, sectlen)) {
                            //
                            // first section isn't the one of interest
                            // so therefore we couldn't find it
                            //
                            curptr = NULL;
                        }
                    }
                }
            }
            if (curptr == NULL) {
                //
                // no matching section found (or file was empty)
                // copy the section header to the end of the file
                // eof is known to be actual end of file
                //
                memcpy(eof, Section, sectlen);
                eof += sectlen;
                memcpy(eof, "\r\n", 2);
                eof += 2;

            } else if (lastsect != NULL) {
                //
                // we have to rearrange the sections, as we have a case as follows:
                //
                // ....
                // ....
                // (curptr) [section A]     = section of interest
                // ....
                // ....
                // (lastsect) [section B]   = section after section of interest
                // ....
                // ....
                //
                // we want to move the text between curptr and lastsect to end of file
                //
                PSTR buffer = MyMalloc((DWORD)(lastsect - curptr));

                if (buffer) {
                    // first copy the important section to the buffer
                    //
                    memcpy(buffer, curptr, (size_t)(lastsect - curptr));
                    //
                    // now move the rest of the thing back
                    //
                    memcpy(curptr, lastsect, (size_t)(eof - lastsect));
                    //
                    // put the important section at the end where it belongs
                    //
                    memcpy(curptr - lastsect + eof, buffer, (size_t)(lastsect - curptr));

                    MyFree(buffer);

                } else {
                    //
                    // BUGBUG!!! (jamiehun) need to do a better default action
                    //
                    // For some reason, we cannot allocate enough memory.
                    //
                    // There are 4 options here:
                    // 1. Do nothing; this will cause the entry to be appended to
                    //    the file, but as part of the wrong section.
                    // 2. Bail; this will cause the log entry to get lost.
                    // 3. Create a second file to contain a temporary copy of the
                    //    section; this will require creating another file, and
                    //    then deleting it.
                    // 4. Extend the mapping of the current file to be big enough
                    //    to hold another copy of the section; this will cause the
                    //    file to have a lot of 0s or possibly another copy of the
                    //    section, should the machine crash during the processing.
                    //
                    // BAIL!
                    //
                    retval = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }
            }
        }

        //
        // now append the log entry
        //
        memcpy(eof, Entry, entrylen);
        eof += entrylen;
        if (eof[-1] != '\n') {
            //
            // entry did not supply en end of line, so we will
            //
            memcpy(eof, "\r\n", 2);
            eof += 2;
        }
        //
        // because of the memory mapping, the file size will not be correct,
        // so set the pointer to where we think the end of file is, and then
        // the real EOF will be set after unmapping, but before closing
        //
        fpoff = SetFilePointer(
            hLogfile,           // handle of file
            (LONG)(eof - baseaddr), // number of bytes to move file pointer
            NULL,               // pointer to high-order DWORD of
                                // distance to move
            FILE_BEGIN);        // how to move

        if (fpoff == (DWORD)(-1) && (error = GetLastError()) != NO_ERROR) {
            MYTRACE(TEXT("SFP returned %u; eof = %u\n"), error, (eof - baseaddr));
            retval = error;
            leave;
        }
        seteof = TRUE;
        retval = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // invalid data
        //
        retval = ERROR_INVALID_DATA;
    }

    //
    // unmap
    //
    if (mapped) {
        UnMapLogFile(baseaddr, hLogfile, hMapping, locksize, seteof);
    }

    return retval;
}

VOID
WriteLogSectionEntry(
    IN PCTSTR FileName,
    IN PCTSTR Section,
    IN PCTSTR Entry,
    IN BOOL SimpleAppend
    )

/*++

Routine Description:

    Convert parameters to ANSI, then append an entry to a given section of the
    log file.

Arguments:

    FileName - supplies the path name of the log file.

    Section - supplies the name of section.

    Entry - supplies the string to append to section.

    SimpleAppend - specifies whether entries will simply be appended to the log
        file or appended to the section where they belong.

Return Value:

    NONE.

--*/

{
    PCSTR ansiSection = NULL;
    PCSTR ansiEntry = NULL;

    try {
        MYASSERT(Section != NULL && Entry != NULL);

#ifdef UNICODE
        ansiSection = UnicodeToMultiByte(Section, CP_ACP);
        ansiEntry = UnicodeToMultiByte(Entry, CP_ACP);

        if(!ansiSection || !ansiEntry) {
            leave;
        }
#else
        ansiSection = Section;
        ansiEntry = Entry;
#endif

        AppendLogEntryToSection(
            FileName,
            ansiSection,
            ansiEntry,
            SimpleAppend);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // invalid data
        //
    }
#ifdef UNICODE
    if (ansiSection != NULL) {
        MyFree(ansiSection);
    }
    if (ansiEntry != NULL) {
        MyFree(ansiEntry);
    }
#endif

}

DWORD
MakeUniqueName(
    IN  PCTSTR Component,        OPTIONAL
    OUT PTSTR * UniqueString
    )

/*++

Routine Description:

    Create a section name that's unique by using a timestamp.
    If Component is supplied, append that to the timestamp.

Arguments:

    Component - supplies a string to be included in the unique name.
    UniqueString - supplies a pointer to be set with return string

Return Value:

    Error status

--*/

{
    SYSTEMTIME now;
    LPTSTR buffer = NULL;
    DWORD status = ERROR_INVALID_DATA;
    ULONG sz;
    LONG UID;

    try {
        if (UniqueString == NULL) {
            //
            // invalid param
            //
            status = ERROR_INVALID_PARAMETER;
            leave;
        }
        *UniqueString = NULL;

        if (Component == NULL) {
            //
            // treat as empty string
            //
            Component = TEXT("");
        }

        UID = InterlockedIncrement(&GlobalUID); // returns a new ID value whenever called, ensures uniqueness per process

        //
        // calculate how big string is going to be, be generous (see wsprintf below)
        //
        sz = /*[] and padding*/ 4 /*date*/ +5+3+3 /*time*/ +3+3+3 /*PID*/ +12 /*UID*/ +12 /*Component*/ +1+lstrlen(Component);
        buffer = MyMalloc(sz * sizeof(TCHAR));
        if (buffer == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        GetLocalTime(&now);

        wsprintf(buffer, TEXT("[%04d/%02d/%02d %02d:%02d:%02d %u.%u%s%s]"),
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond,
            (UINT)GetCurrentProcessId(),
            (UINT)UID,
            (Component[0] ? TEXT(" ") : TEXT("")),
            Component);

        *UniqueString = buffer;
        buffer = NULL;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // status remains ERROR_INVALID_DATA
        //
    }

    if (buffer != NULL) {
        MyFree(buffer);
    }

    return status;
}

DWORD
GetLogSettings(
    IN PSETUP_LOG_CONTEXT LogContext
    )

/*++

Routine Description:

    Get the settings for the log from the registry.

    HKLM\System\Software\Microsoft\Windows\CurrentVersion\Setup

    LogLevel (DWORD) = default flags
            if obmitted or 0, defaults to logging warnings or errors
    LogPath (SZ) = <directory>\<file>
            if <file> is obmitted, "setupapi.log" is assumed
            if LogPath is obmitted, places in windows directory
    AppLogLevels\
        appname (DWORD) = flags
            eg: rundll32.exe = 0x4040
            overrides the default for the specific application

    Flag values:
        see cntxtlog.h
        0x0000FFFF turns on all logging levels
        0x00000000 uses defaults (which is currently warning/errors)
        0x00000101 practically turns off logging
        0x10000000 logs all context info
        0x40000000 generates a chronological log
        0x80000000 logs to debugger as well as to file

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to get settings for

Return Value:

    NONE.

--*/

{
    HKEY key;
    HKEY loglevel;
    DWORD error;
    DWORD level = 0;
    DWORD type;
    DWORD len;
    BOOL simpapp = FALSE;
    BOOL debug = FALSE;
    TCHAR filename[MAX_PATH*2];     // over-generous to save overflow checking work below
    DWORD status = ERROR_INVALID_DATA;
    BOOL isdir = FALSE;
    TCHAR testchar;

    filename[0]=0;

    try {
        error = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGSTR_PATH_SETUP REGSTR_KEY_SETUP,
            0,                  // reserved
            KEY_QUERY_VALUE,
            &key);

        if (error == ERROR_SUCCESS) {

            len = sizeof(level);
            error = RegQueryValueEx(
                key,
                SP_REGKEY_LOGLEVEL,
                0,              // reserved
                &type,
                (LPBYTE)&level, // pointer to data
                &len);

            if (error == ERROR_SUCCESS && type != REG_DWORD) {
                //
                // somebody put crap in this value, so just ignore it
                //
                level = 0;
            }

            len = MAX_PATH * sizeof(TCHAR); // in bytes, not chars
            error = RegQueryValueEx(
                key,
                SP_REGKEY_LOGPATH,
                0,              // reserved
                &type,
                (LPBYTE) filename,
                &len);

            if (error != ERROR_SUCCESS || type != REG_SZ) {
                //
                // somebody put crap in this value, so just ignore it
                // also fails if buffer isn't big enough
                //
                filename[0] = TEXT('\0');
            }

            //
            // Allow a user to override the log level for a particular program
            //
            error = RegOpenKeyEx(
                key,
                SP_REGKEY_APPLOGLEVEL,
                0,                  // reserved
                KEY_QUERY_VALUE,
                &loglevel);

            if (error == ERROR_SUCCESS) {
                DWORD override = 0;
                len = sizeof(override);

                error = RegQueryValueEx(
                    loglevel,
                    MyGetFileTitle(ProcessFileName),
                    0,             // reserved
                    &type,
                    (LPBYTE) &override,
                    &len);

                if (error == ERROR_SUCCESS && type == REG_DWORD) {
                    level = override;
                }

                RegCloseKey(loglevel);
            }

            RegCloseKey(key);
        }

        isdir = FALSE;
        //
        // if they don't supply a valid name, we use the Windows dir
        //
        if (filename[0] == 0) {
            lstrcpyn(filename, WindowsDirectory,MAX_PATH);
            isdir = TRUE; // we know this should be a directory
        }

        //
        // see if we're pointing at a directory
        //
        testchar = CharPrev(filename,filename+lstrlen(filename))[0];
        if(testchar == TEXT('\\') || testchar == TEXT('/')) {
            //
            // explicit directiory
            //
            isdir = TRUE;
        } else {
            DWORD attr = GetFileAttributes(filename);
            if (isdir || (attr != (DWORD)(-1) && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0 )) {
                //
                // implicit directory
                //
                isdir = TRUE;
                lstrcat(filename, TEXT("\\"));
            }
        }

        if (isdir) {
            //
            // if they gave a directory, add a filename
            //
            lstrcat(filename, SP_LOG_FILENAME);
        }
        pSetupMakeSurePathExists(filename);

        //
        // validate level flags
        //
        level &= SETUP_LOG_VALIDREGBITS;
        //
        // if the user wants output sent to the debugger, they set a bit
        // in the LogLevel
        //
        if (level & SETUP_LOG_DEBUGOUT) {
            debug = TRUE;
            level &= ~SETUP_LOG_DEBUGOUT;
        }

        if (level & SETUP_LOG_SIMPLE) {
            simpapp = TRUE;
            level &= ~SETUP_LOG_SIMPLE;
        }

        if((level & SETUP_LOG_LEVELMASK) == 0) {
            //
            // level not explicitly set
            //
            level |= SETUP_LOG_DEFAULT;
        }

        if((level & DRIVER_LOG_LEVELMASK) == 0) {
            //
            // level not explicitly set
            //
            level |= DRIVER_LOG_DEFAULT;
        }

        LogContext->FileName = DuplicateString(filename);
        if (LogContext->FileName == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        LogContext->DebuggerOutput = debug;

        LogContext->SimpleAppend = simpapp;

        LogContext->LogLevel = level;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // status remains ERROR_INVALID_DATA
        //
    }

    return status;
}

DWORD
CreateLogContext(
    IN  PCTSTR SectionName,              OPTIONAL
    OUT PSETUP_LOG_CONTEXT *LogContext
    )

/*++

Routine Description:

    Creates and initializes a SETUP_LOG_CONTEXT struct.

Arguments:

    SectionName - supplies an initial string to be used as part of the
        section name.

    LogContext - supplies a pointer to where the pointer to the allocated
        SETUP_LOG_CONTEXT should be stored.

Return Value:

    NO_ERROR in case of successful structure creation.

    Win32 error code in case of error.

--*/

{
    PSETUP_LOG_CONTEXT lc = NULL;
    DWORD status = ERROR_INVALID_DATA;
    DWORD rc;

    try {

        if (LogContext == NULL) {
            status = ERROR_INVALID_PARAMETER;
            leave;
        }
        *LogContext = NULL;

        lc = (PSETUP_LOG_CONTEXT) MyMalloc(sizeof(SETUP_LOG_CONTEXT));
        if (lc == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        //
        // all fields start out at 0
        //
        ZeroMemory(lc, sizeof(SETUP_LOG_CONTEXT));
        lc->RefCount = 1;
        lc->ContextInfo = NULL;
        lc->ContextIndexes = NULL;
        lc->ContextBufferSize = 0;
        lc->ContextLastUnused = -1;
        lc->ContextFirstUsed = -1;
        lc->ContextFirstAuto = -1;

        //
        // Setup mutex for synchronization (BUGBUG!!! is this needed?)
        //
        if(!InitializeSynchronizedAccess(&lc->Lock)) {
            //
            // if this doesn't initialize, we can't rely on DeleteLogContext to cleanup
            //
            MyFree(lc);
            lc = NULL;
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // BUGBUG!!! call this with GetCommandLine when SectionName is NULL
        //
        rc = MakeUniqueName(SectionName,&(lc->SectionName));
        if (rc != NO_ERROR) {
            status = rc;
            leave;
        }

        //
        // Get FileName, DebuggerOutput, LogLevel from registry...
        //
        rc = GetLogSettings(lc);
        if (rc != NO_ERROR) {
            status = rc;
            leave;
        }

        *LogContext = lc;

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // status remains ERROR_INVALID_DATA
        //
    }

    if (status != NO_ERROR) {
        if (lc != NULL) {
            DeleteLogContext(lc);
            lc = NULL;
        }
    }

    return status;
}

DWORD
AllocLogInfoSlotOrLevel(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD              Level,
    IN BOOL               AutoRelease
    )
/*++

Routine Description:

    Obtain a new context stack entry for a context string only if current logging level is less verbose than specified
    Eg, if we specified DRIVER_LOG_VERBOSE, we will either return DRIVER_LOG_VERBOSE (if we would log it) or a slot
    if we would not normally log it.

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    Level - logging level we want to always log the information at
    AutoRelease - if set, will release the context when dumped

Return Value:

    Slot value to pass to logging functions, or a copy of Level
    note that if there is an error, 0 is returned
    return value can always be passed to ReleaseLogInfoSlot

--*/
{
    if(LogContext == NULL || _WouldNeverLog(LogContext,Level)) {
        //
        // when 0 get's passed to logging functions, it will exit out very quickly
        //
        return 0;
    }
    if(_WouldLog(LogContext,Level)) {
        //
        // Level specifies a verbosity level that would cause logging
        //
        return Level;
    } else {
        //
        // interestingly enough, we will also get here if Level is a slot
        // this is what we want
        //
        return AllocLogInfoSlot(LogContext,AutoRelease);
    }
}

DWORD
AllocLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN BOOL               AutoRelease
    )
/*++

Routine Description:

    Obtain a new context stack entry for a context string

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    AutoRelease - if set, will release the context when dumped

Return Value:

    Slot value to pass to logging functions
    note that if there is an error, 0 is returned
    which may be safely used (means don't log)

--*/
{
    DWORD retval = 0;
    LPVOID newbuffer;
    int newsize;
    int newitem;
    BOOL endsync = FALSE;

    try {
        if (LogContext == NULL) {

            //
            // if they pass no LogContext - duh!
            //
            leave;
        }

        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        if (((LogContext->LogLevel & SETUP_LOG_LEVELMASK) <= SETUP_LOG_NOLOG)
            &&((LogContext->LogLevel & DRIVER_LOG_LEVELMASK) <= DRIVER_LOG_NOLOG)) {
            //
            // BUGBUG!!! (jamiehun)
            // we could potentially optimize this out with a global flag
            //
            leave;
        }

        if (LogContext->ContextLastUnused < 0) {
            //
            // need to allocate more
            //
            if (LogContext->ContextBufferSize >= SETUP_LOG_CONTEXTMASK) {
                //
                // too many contexts
                //
                leave;
            }
            //
            // need to (re)alloc buffer
            //
            newsize = LogContext->ContextBufferSize+10;

            if (LogContext->ContextInfo) {
                newbuffer = MyRealloc(LogContext->ContextInfo,sizeof(PTSTR)*(newsize));
            } else {
                newbuffer = MyMalloc(sizeof(PTSTR)*(newsize));
            }
            if (newbuffer == NULL) {
                leave;
            }
            LogContext->ContextInfo = (PTSTR*)newbuffer;

            if (LogContext->ContextIndexes) {
                newbuffer = MyRealloc(LogContext->ContextIndexes,sizeof(UINT)*(newsize));
            } else {
                newbuffer = MyMalloc(sizeof(UINT)*(newsize));
            }
            if (newbuffer == NULL) {
                leave;
            }
            LogContext->ContextIndexes = (UINT*)newbuffer;
            LogContext->ContextLastUnused = LogContext->ContextBufferSize;
            LogContext->ContextBufferSize ++;
            while(LogContext->ContextBufferSize < newsize) {
                LogContext->ContextIndexes[LogContext->ContextBufferSize-1] = LogContext->ContextBufferSize;
                LogContext->ContextBufferSize ++;
            }
            LogContext->ContextIndexes[LogContext->ContextBufferSize-1] = -1;
        }

        newitem = LogContext->ContextLastUnused;
        LogContext->ContextLastUnused = LogContext->ContextIndexes[newitem];

        if(AutoRelease) {
            if (LogContext->ContextFirstAuto<0) {
                //
                // first auto-release context item
                //
                LogContext->ContextFirstAuto = newitem;
            } else {
                int lastitem = LogContext->ContextFirstAuto;
                while (LogContext->ContextIndexes[lastitem]>=0) {
                    lastitem = LogContext->ContextIndexes[lastitem];
                }
                LogContext->ContextIndexes[lastitem] = newitem;
            }
        } else {
            if (LogContext->ContextFirstUsed<0) {
                //
                // first context item
                //
                LogContext->ContextFirstUsed = newitem;
            } else {
                int lastitem = LogContext->ContextFirstUsed;
                while (LogContext->ContextIndexes[lastitem]>=0) {
                    lastitem = LogContext->ContextIndexes[lastitem];
                }
                LogContext->ContextIndexes[lastitem] = newitem;
            }
        }
        LogContext->ContextIndexes[newitem] = -1;   // init
        LogContext->ContextInfo[newitem] = NULL;

        retval = (DWORD)(newitem) | SETUP_LOG_IS_CONTEXT;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
        retval = 0;
    }

    if (endsync) {

        try {
            //
            // we need to end access as we haven't done so
            //
            EndSynchronizedAccess(&LogContext->Lock);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing special; this just allows us to catch errors
            //
        }
    }

    //
    // returns a logging flag (SETUP_LOG_IS_CONTEXT | n) or 0
    //
    return retval;
}

VOID
ReleaseLogInfoSlot(
    IN PSETUP_LOG_CONTEXT LogContext,
    DWORD Slot
    )
/*++

Routine Description:

    Releases (non auto-release) slot previously obtained

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    Slot - supplies Slot value returned by AllocLogInfoSlot

Return Value:

    none

--*/
{
    int item;
    int lastitem;
    BOOL endsync = FALSE;

    if ((Slot & SETUP_LOG_IS_CONTEXT) == 0) {
        //
        // GetLogContextMark had failed, value wasn't set, or not a context log
        //
        return;
    }

    try {
        //
        // log context must have been supplied
        //

        MYASSERT(LogContext != NULL);

        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        item = (int)(Slot & SETUP_LOG_CONTEXTMASK);

        MYASSERT(item >= 0);
        MYASSERT(item < LogContext->ContextBufferSize);
        MYASSERT(LogContext->ContextFirstUsed >= 0);

        //
        // remove item out of linked list
        //

        if (item == LogContext->ContextFirstUsed) {
            //
            // removing first in list
            //
            LogContext->ContextFirstUsed = LogContext->ContextIndexes[item];
        } else {
            lastitem = LogContext->ContextFirstUsed;
            while (lastitem >= 0) {
                if (LogContext->ContextIndexes[lastitem] == item) {
                    LogContext->ContextIndexes[lastitem] = LogContext->ContextIndexes[item];
                    break;
                }
                lastitem = LogContext->ContextIndexes[lastitem];
            }
        }

        //
        // drop a string that hasn't been output
        //

        if (LogContext->ContextInfo[item] != NULL) {
            MyFree(LogContext->ContextInfo[item]);
            LogContext->ContextInfo[item] = NULL;
        }

        //
        // add item into free list
        //

        LogContext->ContextIndexes[item] = LogContext->ContextLastUnused;
        LogContext->ContextLastUnused = item;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
    }

    if (endsync) {

        try {
            //
            // we need to end access as we haven't done so
            //
            EndSynchronizedAccess(&LogContext->Lock);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing special; this just allows us to catch errors
            //
        }
    }

}

VOID
ReleaseLogInfoList(
    IN     PSETUP_LOG_CONTEXT LogContext,
    IN OUT PINT               ListStart
    )
/*++

Routine Description:

    Releases whole list of slots

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to use
    ListStart - pointer to list index

Return Value:

    none

--*/
{
    int item;
    BOOL endsync = FALSE;

    MYASSERT(ListStart);

    try {
        if (*ListStart < 0) {
            //
            // list is empty
            //
            leave;
        }

        //
        // log context must have been supplied
        //

        MYASSERT(LogContext != NULL);

        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        while (*ListStart >= 0) {
            item = *ListStart;                                  // item we're about to release
            MYASSERT(item < LogContext->ContextBufferSize);
            *ListStart = LogContext->ContextIndexes[item];      // next item on list (we're going to trash this index)

            if (LogContext->ContextInfo[item] != NULL) {
                MyFree(LogContext->ContextInfo[item]);          // release string if still allocated
                LogContext->ContextInfo[item] = NULL;
            }

            //
            // add to free list
            //
            LogContext->ContextIndexes[item] = LogContext->ContextLastUnused;
            LogContext->ContextLastUnused = item;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
    }

    if (endsync) {

        try {
            //
            // we need to end access as we haven't done so
            //
            EndSynchronizedAccess(&LogContext->Lock);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing special; this just allows us to catch errors
            //
        }
    }

}

VOID
DeleteLogContext(
    IN PSETUP_LOG_CONTEXT LogContext
    )

/*++

Routine Description:

    Decrement ref count of LogContext, and delete if zero.

Arguments:

    LogContext - supplies a pointer to the SETUP_LOG_CONTEXT to be deleted.

Return Value:

    NONE.

--*/

{
    BOOL endsync = FALSE;
    try {
        if (!LogContext) {
            leave;
        }

        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        //
        // check ref count
        //
        MYASSERT(LogContext->RefCount > 0);
        if (--LogContext->RefCount) {
            // ref count is > 0, so the struct is still in use
            EndSynchronizedAccess(&LogContext->Lock);
            leave;
        }

        ReleaseLogInfoList(LogContext,&LogContext->ContextFirstAuto);
        ReleaseLogInfoList(LogContext,&LogContext->ContextFirstUsed);

        if (LogContext->SectionName) {
            MyFree(LogContext->SectionName);
        }

        if (LogContext->FileName) {
            MyFree(LogContext->FileName);
        }

        if (LogContext->Buffer) {
            MyFree(LogContext->Buffer);
        }

        if (LogContext->ContextInfo) {
            MyFree(LogContext->ContextInfo);
        }

        if (LogContext->ContextIndexes) {
            MyFree(LogContext->ContextIndexes);
        }

        //
        // get rid of the lock (unlock it first!)
        //
        endsync = FALSE;
        EndSynchronizedAccess(&LogContext->Lock);
        DestroySynchronizedAccess(&LogContext->Lock);

        //
        // now deallocate the struct
        //
        MyFree(LogContext);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        // we shouldn't fail unless LogContext is bad
        // in which case there's no point trying to free
        //
    }
    if (endsync) {
        //
        // something went wrong, but we had lock
        //
        EndSynchronizedAccess(&LogContext->Lock);
    }

    return;
}

DWORD
RefLogContext(  // increment reference count
    IN PSETUP_LOG_CONTEXT LogContext
    )

/*++

Routine Description:

    Increment the reference count on a SETUP_LOG_CONTEXT object.


Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object. If
        NULL, this is a NOP.

Return Value:

    DWORD containing old reference count.

--*/

{
    DWORD ref = 0;
    BOOL endsync = FALSE;

    if (LogContext == NULL) {
        return 0;
    }

    try {
        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        ref = LogContext->RefCount++;
        MYASSERT(LogContext->RefCount);


    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }
    if(endsync) {
        EndSynchronizedAccess(&LogContext->Lock);
    }

    return ref;
}

VOID
SendLogString(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR Buffer
    )

/*++

Routine Description:

    Send a string to the logfile and/or debugger based on settings.

Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object.

    Buffer - supplies the buffer to be sent to the logfile/debugger.

Return Value:

    NONE.

--*/

{
    int len;

    try {
        MYASSERT(LogContext);
        MYASSERT(Buffer);

        if (Buffer[0] == 0) {
            //
            // useless call
            //
            leave;
        }

        if (LogContext->FileName) {
            WriteLogSectionEntry(
                LogContext->FileName,
                LogContext->SectionName,
                Buffer,
                LogContext->SimpleAppend);
        }

        //
        // do debugger output here
        //
        if (LogContext->DebuggerOutput) {
            DebugPrint(
                TEXT("SetupAPI: %s: %s"),
                LogContext->SectionName,
                Buffer);
            len = lstrlen(Buffer);
            if (Buffer[len-1] != TEXT('\n')) {
                DebugPrint(TEXT("\r\n"));
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }
}

DWORD
WriteLogEntry(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD MessageId,
    IN PCTSTR MessageStr,               OPTIONAL
    ...                                 OPTIONAL
    )

/*++

Routine Description:

    Write a log entry to a file or debugger. If MessageId is 0 and MessageStr
    is NULL, the LogContext's buffer will be flushed.

Arguments:

    LogContext - optionally supplies a pointer to the SETUP_LOG_CONTEXT to be
        used for logging. If not supplied, a temporary one is created just for
        a single use.

    Level - bitmask indicating logging flags. See SETUP_LOG_* and DRIVER_LOG_*
        at the beginning of cntxtlog.h for details. It may also be a slot
        returned by AllocLogInfoSlot, or 0 (no logging)

    MessageId - ID of string from string table. Ignored if MessageStr is
        supplied. The string may contain formatting codes for FormatMessage.

    MessageStr - optionally supplies string to be formatted with FormatMessage.
        If not supplied, MessageId is used instead.

    ... - supply optional parameters based on string to be formatted.

Return Value:

    Win32 error code.

--*/

{
    PSETUP_LOG_CONTEXT lc = NULL;
    DWORD retval = NO_ERROR;
    DWORD error;
    DWORD flags;
    DWORD context = 0;
    DWORD i;
    DWORD logmask;
    DWORD count;
    LPVOID source = NULL;
    PTSTR buffer = NULL;
    PTSTR buffer2 = NULL;
    va_list arglist;
    BOOL localstr = TRUE;
    BOOL logit = FALSE;
    BOOL timestamp = FALSE;
    BOOL endsync = FALSE;
    SYSTEMTIME now;

    try {
        //
        // return immediately if we're presented by a no-logging index
        // or (if we have a context) that the context specifies no logging
        //
        if (_WouldNeverLog(LogContext,Level)) {
            retval = NO_ERROR;
            leave;
        }
        //
        // if no log context was supplied, we'll make a temp one
        //
        if (LogContext == NULL) {
            //
            // if they pass no LogContext and they want buffering, this call's a nop
            //
            if (Level & SETUP_LOG_BUFFER) {
                retval = NO_ERROR;
                leave;
            }

            error = CreateLogContext(NULL, &lc);
            if (error != NO_ERROR) {
                lc = NULL;
                retval = error;
                leave;
            }

            LogContext = lc;
            //
            // BugBug!!! (jamiehun)
            // shame to find out now in the case of LogContext originally NULL
            // fix at later date
            //
            if (_WouldNeverLog(LogContext,Level)) {
                retval = NO_ERROR;
                leave;
            }
        }

        if ((Level & SETUP_LOG_IS_CONTEXT)!=0) {
            //
            // context logging
            //
            if(Level & ~SETUP_LOG_VALIDCONTEXTBITS) {
                MYASSERT((Level & ~SETUP_LOG_VALIDCONTEXTBITS)==0);
                retval = ERROR_INVALID_PARAMETER;
                leave;
            }
            context = Level & SETUP_LOG_CONTEXTMASK;
            Level = SETUP_LOG_IS_CONTEXT;   // effective log level, we've stripped out log context
            logit = TRUE;                   // make sure we don't decide to ignore error
            if ((LogContext->LogLevel & SETUP_LOG_ALL_CONTEXT)!=0) {
                //
                // don't treat as context
                //
                Level = 0;
            }
        }

        if(!logit) {
            //
            // we're not logging due to context info, let's see if we should log this based on level rules
            //
            logit = _WouldLog(LogContext,Level);
            if (!logit) {
                //
                // oh well, we didn't want this anyway
                //
                leave;
            }
        }

        //
        // up until this point we've been looking at LogContext->LogLevel
        // which is pretty safe
        // after this point, we know we're going to log something, so it's going to be slow anyway
        //
        if (! BeginSynchronizedAccess(&LogContext->Lock)) {
            retval = ERROR_INVALID_DATA;
            leave;
        }
        endsync = TRUE; // indicate we need to End later

        if (((Level & DRIVER_LOG_LEVELMASK) >= DRIVER_LOG_TIME) || ((Level & SETUP_LOG_LEVELMASK) >= SETUP_LOG_TIME)) {
            //
            // time-stamp verbosity
            //
            timestamp = TRUE;
        }
        if (timestamp == FALSE && (Level & SETUP_LOG_LEVELMASK) > 0 && SETUP_LOG_TIMEALL <= (LogContext->LogLevel & SETUP_LOG_LEVELMASK)) {
            //
            // we're interested in time-stamping all raw log-level events
            //
            timestamp = TRUE;
        }
        if (timestamp == FALSE && (Level & DRIVER_LOG_LEVELMASK) > 0 && DRIVER_LOG_TIMEALL <= (LogContext->LogLevel & DRIVER_LOG_LEVELMASK)) {
            //
            // we're interested in time-stamping all raw driver-level events
            //
            timestamp = TRUE;
        }

        if ((Level & SETUP_LOG_IS_CONTEXT) == FALSE) {
            //
            // only do this if we're about to do REAL logging
            //
            // if this is the first log output in the process, we will give the
            // command line and module to help the user see what's going on
            //
            if (InterlockedIncrement(&GlobalLogCount) == 1) {
                TCHAR modbuf[512];
                //
                // recursively call ourselves
                //
                // BUGBUG!!! (jamiehun) Using GetCommandLineA is a hack, and is there because
                // rundll32 trashes the Unicode command line string (boo).
                // MSG_LOG_COMMAND_LINE uses !hs! to display the ANSI string
                //
                WriteLogEntry(
                    LogContext,
                    AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                    MSG_LOG_COMMAND_LINE,
                    NULL,
                    GetCommandLineA());

                if (GetModuleFileName(NULL, modbuf, sizeof(modbuf) / sizeof(TCHAR))) {
                    WriteLogEntry(
                        LogContext,
                        AllocLogInfoSlot(LogContext,TRUE),  // delayed slot
                        MSG_LOG_MODULE_NAME,
                        NULL,
                        modbuf);
                }
            }
        }

        flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;

        //
        // if MessageStr is supplied, we use that; otherwise use a
        // string from a string table
        //
        if (MessageStr) {
            flags |= FORMAT_MESSAGE_FROM_STRING;
            source = (PTSTR) MessageStr;    // cast away const
        } else if (MessageId) {
            //
            // the message ID may be an HRESULT error code
            //
            if (MessageId & 0xC0000000) {
                flags |= FORMAT_MESSAGE_FROM_SYSTEM;
                //
                // Some system messages contain inserts, but whomever is calling
                // will not supply them, so this flag prevents us from
                // tripping over those cases.
                //
                flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
            } else {
                flags |= FORMAT_MESSAGE_FROM_HMODULE;
                source = MyDllModuleHandle;
            }
        }

        if (MessageStr || MessageId) {
            va_start(arglist, MessageStr);
            count = FormatMessage(
                        flags,
                        source,
                        MessageId,
                        0,              // LANGID
                        (LPTSTR) &buffer,
                        0,              // minimum size of buffer
                        &arglist);

        } else {
            //
            // There is no string to format, so we are probably just
            // flushing the buffer.
            //
            count = 1;
        }

        if (count > 0) {
            //
            // no error; output/save the string and free it
            //

            //
            // Check to see if the buffer has anything in it. If so, the newest
            // string needs to be appended to it.
            //
            if (LogContext->Buffer) {
                //
                // in case of a flush, buffer == NULL
                //
                if (buffer!=NULL) {
                    buffer2 = MyRealloc(LogContext->Buffer,
                        (lstrlen(LogContext->Buffer) +
                        lstrlen(buffer) + 1) * sizeof(TCHAR));

                    //
                    // if the realloc was successful, add the new data, otherwise
                    // just drop it on the floor
                    //
                    if (buffer2) {
                        lstrcat(buffer2, buffer);
                        LogContext->Buffer = buffer2;
                        buffer2 = NULL;
                    }

                    LocalFree(buffer);
                    buffer = NULL;
                }
                buffer = LogContext->Buffer;
                //
                // buffer is no longer a LocalAlloc string
                //
                localstr = FALSE;

                LogContext->Buffer = NULL;
            }

            if (Level & SETUP_LOG_BUFFER) {
                //
                // if it's a localstr, make a dup string so we can free buffer later
                //
                if(localstr && buffer != NULL) {
                    LogContext->Buffer = DuplicateString(buffer);
                } else {
                    LogContext->Buffer = buffer;
                    buffer = NULL;
                }

            } else if (Level & SETUP_LOG_IS_CONTEXT) {

                PTSTR TempDupeString;

                //
                // replace the string indicated
                //

                if(buffer) {
                    TempDupeString = DuplicateString(buffer);
                } else {
                    TempDupeString = NULL;
                }

                if(TempDupeString) {
                    if (LogContext->ContextInfo[context]) {
                        MyFree(LogContext->ContextInfo[context]);
                    }
                    LogContext->ContextInfo[context] = TempDupeString;
                }

            } else {
                int item;
                //
                // actually do some logging
                //
                LogContext->LoggedEntries++;

                if (!LogContext->SectionName) {
                     error = MakeUniqueName(NULL,&(LogContext->SectionName));
                }

                //
                // first dump the auto-release context info
                //
                item = LogContext->ContextFirstAuto;

                while (item >= 0) {
                    if (LogContext->ContextInfo[item]) {
                        //
                        // dump this string
                        //
                        SendLogString(LogContext, LogContext->ContextInfo[item]);
                        MyFree (LogContext->ContextInfo[item]);
                        LogContext->ContextInfo[item] = NULL;
                    }
                    item = LogContext->ContextIndexes[item];
                }

                ReleaseLogInfoList(LogContext,&LogContext->ContextFirstAuto);

                //
                // now dump any strings set in currently allocated slots
                //
                item = LogContext->ContextFirstUsed;

                while (item >= 0) {
                    if (LogContext->ContextInfo[item]) {
                        //
                        // dump this string
                        //
                        SendLogString(LogContext, LogContext->ContextInfo[item]);
                        MyFree (LogContext->ContextInfo[item]);
                        LogContext->ContextInfo[item] = NULL;
                    }
                    item = LogContext->ContextIndexes[item];
                }

                //
                // we have built up a line to send
                //
                if (buffer != NULL) {
                    //
                    // this is the point we're interested in prefixing with timestamp
                    // this allows us to build up a string, then emit it prefixed with stamp
                    //
                    if(timestamp) {
                        ULONG sz = /*@ and padding*/ 5 /*time*/ +3+3+3+4 /*Log string*/ +1+lstrlen(buffer);
                        buffer2 = MyMalloc(sz * sizeof(TCHAR));
                        if (buffer2 != NULL) {

                            GetLocalTime(&now);

                            wsprintf(buffer2, TEXT("@ %02d:%02d:%02d.%03d : %s"),
                                now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
                                buffer);

                            if (localstr) {
                                LocalFree(buffer);
                            } else {
                                MyFree(buffer);
                            }
                            //
                            // this becomes the new buffer
                            //
                            buffer = buffer2;
                            buffer2 = NULL;
                            localstr = FALSE;
                        }
                    }
                    //
                    // now send the message we were called to write in the first place
                    //
                    SendLogString(LogContext, buffer);
                }
            }

        } else {
            //
            // the FormatMessage failed
            //
            retval = GetLastError();
            if(retval == NO_ERROR) {
                retval = ERROR_INVALID_DATA;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing special; this just allows us to catch errors
        //
        retval = ERROR_INVALID_DATA;
    }

    if (endsync) {

        try {
            //
            // we need to end access as we haven't done so
            //
            EndSynchronizedAccess(&LogContext->Lock);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing special; this just allows us to catch errors
            //
        }
    }

    //
    // cleanup stuff
    //

    //
    // if the buffer was allocated for us, we have to free it
    //
    if (buffer) {
        if (localstr) {
            LocalFree(buffer);
        } else {
            MyFree(buffer);
        }
    }
    //
    // if we allocated a local log-config, we have to free it
    //
    if (lc) {
        DeleteLogContext(lc);
    }
    return retval;
}

VOID
SetLogSectionName(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Sets the section name for the log context if it hasn't been used.

Arguments:

    LogContext - supplies pointer to SETUP_LOG_CONTEXT.

    SectionName - supplies a pointer to a string to be included in the
        section name.

Return Value:

    NONE.

--*/

{
    DWORD rc;
    PTSTR NewSectionName = NULL;
    BOOL endsync = FALSE;

    MYASSERT(LogContext);
    MYASSERT(SectionName);

    try {
        if(!BeginSynchronizedAccess(&LogContext->Lock)) {
            leave;
        }
        endsync = TRUE;

        //
        // make sure the entry has never been used before
        //
        if (LogContext->LoggedEntries==0 || LogContext->SectionName==NULL) {
            //
            // get rid of any previous name
            //

            rc = MakeUniqueName(SectionName,&NewSectionName);
            if (rc == NO_ERROR) {
                if (LogContext->SectionName) {
                    MyFree(LogContext->SectionName);
                }
                LogContext->SectionName = NewSectionName;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }

    if (endsync) {
        EndSynchronizedAccess(&LogContext->Lock);
    }
}

#if MEM_DBG
#undef InheritLogContext            // defined again below
#endif

DWORD
InheritLogContext(
    IN TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN PSETUP_LOG_CONTEXT Source,
    OUT PSETUP_LOG_CONTEXT *Dest
    )

/*++

Routine Description:

    Copies a log context from one structure to another, deleting the one that
    gets overwritten. If Source and Dest are both NULL, a new log context is
    created for Dest.

Arguments:

    Source - supplies pointer to source SETUP_LOG_CONTEXT. If NULL, this
        creates a new log context for Dest.

    Dest - supplies the location to receive a pointer to the log context.

Return Value:

    NONE.

--*/

{
    DWORD status = ERROR_INVALID_DATA;
    DWORD rc;
    PSETUP_LOG_CONTEXT Old = NULL;

    TRACK_PUSH

    try {
        MYASSERT(Dest);
        Old = *Dest;
        if (Old == NULL && Source == NULL) {
            //
            // this is a roundabout way of saying we want to create a context
            // used when the source logcontext is optional
            //
            rc = CreateLogContext(NULL, Dest);
            if (rc != NO_ERROR) {
                status = rc;
                leave;
            }
        } else if (Source != NULL && (Old == NULL || Old->LoggedEntries == 0)) {
            //
            // We can replace Dest, since it hasn't been used yet
            //
            *Dest = Source;
            RefLogContext(Source);
            if (Old != NULL) {
                //
                // now delete old
                //
                DeleteLogContext(Old);
            }
        }

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    TRACK_POP

    return status;
}

#if MEM_DBG
#define InheritLogContext(a,b)          InheritLogContext(TRACK_ARG_CALL,a,b)
#endif


VOID
WriteLogError(
    IN PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    IN DWORD Level,
    IN DWORD Error
    )

/*++

Routine Description:

    Logs an error code and an error message on the same line.

Arguments:

    LogContext - supplies a pointer to a valid SETUP_LOG_CONTEXT object. If
        NULL, this is a NOP.

    Level - supplies a log level as defined by WriteLogEntry.

    Error - supplies the Win32 error, HRESULT, or SETUPAPI error code to log.

Return Value:

    NONE.

--*/

{
    DWORD err;

    if (!LogContext) {
        //
        // error is meaningless without context
        //
        return;
    }

    WriteLogEntry(
        LogContext,
        Level | SETUP_LOG_BUFFER,
        //
        // print HRESULTs in hex, Win32 errors in decimal
        //
        (Error & 0xC0000000 ? MSG_LOG_HRESULT_ERROR
                            : MSG_LOG_WIN32_ERROR),
        NULL,
        Error);

    //
    // If it's a Win32 error, we convert it to an HRESULT, because
    // WriteLogEntry only knows that it's an error code by the fact
    // that it's an HRESULT.  However, we don't want the user to
    // get an HRESULT if we can help it, so just do the conversion
    // after converting to a string. Also, SETUPAPI errors are not
    // in proper HRESULT format without conversion
    //
    Error = HRESULT_FROM_SETUPAPI(Error);

    //
    // writing the error message may fail...
    //
    err = WriteLogEntry(
        LogContext,
        Level,
        Error,
        NULL);

    if (err != NO_ERROR) {
        WriteLogEntry(
            LogContext,
            Level,
            MSG_LOG_UNKNOWN_ERROR,
            NULL);
    }
}
