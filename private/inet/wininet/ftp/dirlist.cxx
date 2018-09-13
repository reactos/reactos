/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dirlist.cxx

Abstract:

    Contains functions for parsing directory output from LIST command

    Contents:
        ParseDirList
        IsFilespecWild
        ClearFindList
        (DetermineDirectoryFormat)
        (IsNtDateFormat)
        (GetToken)
        (IsUnixAttributeFormat)
        (ParseNtDirectory)
        (ParseUnixDirectory)
        (ParseOs2Directory)
        (ParseMacDirectory)
        (ExtractFileSize)
        (_ExtractFilename)
        (ExtractNtDate)
        (ExtractUnixDate)
        (ExtractOs2Attributes)
        (ParseWord)
        (ExtractInteger)

Author:

    Richard L Firth (rfirth) 26-Jul-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    26-Jul-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// private manifests
//

#define MAX_YEAR_SUPPORTED  2100
#define TOKEN_BUFFER_LENGTH 128
#define RELATIVELY_SMALL_AMOUNT_OF_LS_DATA  512 // arbitrary, but allow for
                                                // prolix error text

//
// private types
//

typedef enum {
    State_Start,
    State_Error,
    State_Continue,
    State_Done
} PARSE_STATE;

typedef PARSE_STATE (*DIR_PARSER)(LPSTR*, LPDWORD, LPWIN32_FIND_DATA);

//
// private macros
//

#define ClearFileTime(fileTime) \
    (fileTime).dwLowDateTime = 0; \
    (fileTime).dwHighDateTime = 0;

#define ClearFindDataFields(lpFind) \
    ClearFileTime((lpFind)->ftCreationTime); \
    ClearFileTime((lpFind)->ftLastAccessTime); \
    (lpFind)->dwReserved0 = 0; \
    (lpFind)->dwReserved1 = 0; \
    (lpFind)->cAlternateFileName[0] = '\0';

//
// private prototypes
//

PRIVATE
BOOL
DetermineDirectoryFormat(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    OUT DIR_PARSER* ParserFunction
    );

PRIVATE
BOOL
IsNtDateFormat(
    IN LPSTR lpBuffer,
    IN DWORD dwBufferLength
    );

PRIVATE
BOOL
GetToken(
    IN LPSTR lpszBuffer,
    IN DWORD dwBufferLength,
    OUT LPSTR lpszToken,
    IN OUT LPDWORD lpdwTokenLength
    );

PRIVATE
BOOL
IsUnixAttributeFormat(
    IN LPSTR lpBuffer,
    IN DWORD dwBufferLength
    );

PRIVATE
PARSE_STATE
ParseNtDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
PARSE_STATE
ParseUnixDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
PARSE_STATE
ParseOs2Directory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
PARSE_STATE
ParseMacDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
ExtractFileSize(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
_ExtractFilename(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
ExtractNtDate(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
ExtractUnixDate(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
ExtractOs2Attributes(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    );

PRIVATE
BOOL
ParseWord(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN WORD LowerBound,
    IN WORD UpperBound,
    OUT LPWORD lpNumber
    );

PRIVATE
BOOL
ExtractInteger(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    OUT LPINT lpNumber
    );

//
// private data
//

//
// DefaultSystemTime - if we fail to parse the time/date field for any reason,
// we will return this default time
//

PRIVATE static SYSTEMTIME DefaultSystemTime = {1980, 1, 0, 1, 12, 0, 0, 0};

//
// functions
//

DWORD
ParseDirList(
    IN LPSTR lpBuffer,
    IN DWORD dwBufferLength,
    IN LPSTR lpszFilespec OPTIONAL,
    IN OUT PLIST_ENTRY lpList
    )

/*++

Routine Description:

    Creates a list of WIN32_FIND_DATA structures given the output from the LIST
    command run at the FTP server

Arguments:

    lpBuffer        - pointer to buffer containing LIST output

    lpBufferLength  - length of Buffer - no trailing \0

    lpszFilespec    - pointer to file specification used to generate listing.
                      May contain path components. May be NULL, in which case
                      we perform no filtering based on name (results should be
                      an exact match with request)

    lpList          - pointer to LIST_ENTRY list to add to

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "ParseDirList",
                "%x, %d, %x",
                lpBuffer,
                dwBufferLength,
                lpList
                ));

    DWORD error;
    DIR_PARSER directoryParser;
    BOOL needBuffer;
    LPSTR lpszOriginalBuffer;
    DWORD dwOriginalBufferLength;

    //
    // remember the initial buffer pointer and length, in case we can't determine
    // the format - DetermineDirectoryFormat() will alter the input buffer pointer
    // and length
    //

    lpszOriginalBuffer = lpBuffer;
    dwOriginalBufferLength = dwBufferLength;

    //
    // find out the format of the directory listing. Currently we understand
    // NT and the basic Unix directory listing formats
    //

    if (!DetermineDirectoryFormat(&lpBuffer, &dwBufferLength, &directoryParser)) {

        DEBUG_PRINT(FTP,
                    ERROR,
                    ("Can't determine directory format\n"
                    ));

        //
        // if we received a relatively small amount of data, then there is a
        // good chance that what we actually received is an error message from
        // the ls command, or operating system, etc. Make it an extended error.
        // This will reduce (but not eliminate) the chances of getting back an
        // internal error
        //

        if (dwBufferLength <= RELATIVELY_SMALL_AMOUNT_OF_LS_DATA) {
            error = InternetSetLastError(0,
                                         lpszOriginalBuffer,
                                         dwOriginalBufferLength,
                                         SLE_APPEND | SLE_ZERO_TERMINATE
                                         );

            //
            // return internal error if we failed to add the text for any reason
            //

            error = (error == ERROR_SUCCESS)
                        ? ERROR_INTERNET_EXTENDED_ERROR
                        : ERROR_INTERNET_INTERNAL_ERROR
                        ;
        } else {

            //
            // BUGBUG - error code?
            //

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
        goto quit;
    }

    //
    // the list must be currently empty
    //

    INET_ASSERT(IsListEmpty(lpList));

    //
    // the app may have specified a path. Chances are that if there are
    // wildcards within the path then the server would have returned an
    // error. But if the app requested e.g. foo\bar\*.exe then the server
    // would have returned the directory results for foo\bar. Therefore,
    // we must skip any path components, or all tests against the filespec
    // will fail
    //

    if (ARGUMENT_PRESENT(lpszFilespec)) {

        LPSTR lpszSpec;

        lpszSpec = strrchr(lpszFilespec, '\\');
        if (lpszSpec == NULL) {
            lpszSpec = strrchr(lpszFilespec, '/');
        }
        if (lpszSpec != NULL) {
            lpszFilespec = lpszSpec + 1;
        }

        DEBUG_PRINT(FTP,
                    INFO,
                    ("lpszFilespec = %s\n",
                    lpszFilespec
                    ));

    }

    //
    // loop round, parsing the listing until we reach the end or get an
    // error
    //

    needBuffer = TRUE;
    error = ERROR_SUCCESS;
    while ((dwBufferLength != 0) && (error == ERROR_SUCCESS)) {

        PLIST_ENTRY dirEntry;
        LPWIN32_FIND_DATA lpFind;

        //
        // we need to allocate a buffer for the WIN32_FIND_DATA structure
        // unless we already have one from the previous iteration (because
        // the filename didn't match our target criteria)
        //

        if (needBuffer) {
            dirEntry = (PLIST_ENTRY)ALLOCATE_FIXED_MEMORY(
                                    sizeof(LIST_ENTRY) + sizeof(WIN32_FIND_DATA)
                                    );
            lpFind = (LPWIN32_FIND_DATA)(dirEntry + 1);
            needBuffer = FALSE;

            DEBUG_PRINT(FTP,
                        INFO,
                        ("Allocated WIN32_FIND_DATA @ %x\n",
                        lpFind
                        ));

        }
        if (dirEntry == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;

            DEBUG_PRINT(FTP,
                        ERROR,
                        ("Failed to allocate WIN32_FIND_DATA\n"
                        ));

        } else {

            PARSE_STATE state;

            //
            // zero initialize the WIN32_FIND_DATA fields we don't fill in
            // below
            //

            ClearFindDataFields(lpFind);

            //
            // and parse the rest of the information out of the returned FTP
            // directory listing
            //

            state = directoryParser(&lpBuffer, &dwBufferLength, lpFind);

            //
            // if the parser returns State_Continue or State_Done then we need
            // to add the structure to the list if the caller wants it, else we
            // free it and quit
            //

            if (state != State_Error) {

                BOOL addIt;

                //
                // before we put this entry on the list, see if the caller wants
                // it
                //

                if (ARGUMENT_PRESENT(lpszFilespec)) {
                    addIt = FsRtlIsNameInExpression(lpszFilespec,
                                                    lpFind->cFileName,
                                                    TRUE    // case-sensitive
                                                    );
                } else {
                    addIt = TRUE;
                }
                if (addIt) {

                    DEBUG_PRINT(FTP,
                                INFO,
                                ("Match: file %q, target %q\n",
                                lpFind->cFileName,
                                lpszFilespec
                                ));

                    InsertTailList(lpList, (PLIST_ENTRY)dirEntry);
                    needBuffer = TRUE;
                } else {

                    DEBUG_PRINT(FTP,
                                INFO,
                                ("No match: file %q, target %q\n",
                                lpFind->cFileName,
                                lpszFilespec
                                ));

                }
            }

            //
            // if we had an error or there's no more buffer to parse but we
            // didn't keep the last entry, then we need to free the unused
            // WIN32_FIND_DATA and get out
            //

            if ((state == State_Error) || ((state == State_Done) && !needBuffer)) {
                FREE_MEMORY(dirEntry);
                if (state == State_Error) {

                    DEBUG_PRINT(FTP,
                                ERROR,
                                ("State_Error\n"
                                ));

                    //
                    // BUGBUG - error code
                    //

                    error = ERROR_INTERNET_INTERNAL_ERROR;
                }
            }
        }
    }

quit:

    //
    // if we had an error then free up any data structures that we allocated
    //

    if (error != ERROR_SUCCESS) {
        ClearFindList(lpList);
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
IsFilespecWild(
    IN LPCSTR lpszFilespec
    )

/*++

Routine Description:

    Returns TRUE if lpszFilespec is a wild-card file specifier

Arguments:

    lpszFilespec    - pointer to string containing file specification. Cannot
                      be a NULL string

Return Value:

    BOOL

--*/

{
    int len;
    int i;

    INET_ASSERT(ARGUMENT_PRESENT(lpszFilespec));

    //
    // check if the file specifier contains a '*' or a '?'. If so, then the
    // caller is making a DOS-style search request and we have to perform our
    // own filtering, otherwise, we can leave the server to return what the
    // caller asked for
    //

    for (i = 0, len = strlen(lpszFilespec); i < len; ++i) {
        if ((lpszFilespec[i] == '*') || (lpszFilespec[i] == '?')) {
            return TRUE;
        }
    }
    return FALSE;
}


VOID
ClearFindList(
    IN PLIST_ENTRY lpList
    )

/*++

Routine Description:

    Dequeues and deallocates all WIN32_FIND_DATA structures on a directory list

Arguments:

    lpList  - pointer to list to clear

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_FTP,
                None,
                "ClearFindList",
                "%x",
                lpList
                ));

    while (!IsListEmpty(lpList)) {

        PLIST_ENTRY lpHead;

        lpHead = RemoveHeadList(lpList);

        DEBUG_PRINT(FTP,
                    INFO,
                    ("Freeing WIN32_FIND_DATA @ %x, FileName=%q\n",
                    lpHead,
                    ((LPWIN32_FIND_DATA)(lpHead + 1))->cFileName
                    ));

        FREE_MEMORY(lpHead);
    }

    DEBUG_LEAVE(0);
}

//
// private functions
//


PRIVATE
BOOL
DetermineDirectoryFormat(
    IN LPSTR* lpBuffer,
    IN LPDWORD lpdwBufferLength,
    OUT DIR_PARSER* lpfnParserFunction
    )

/*++

Routine Description:

    Determines whether the directory listing is in Unix or NT (or other?) format
    and returns a pointer to the parser function to use

    The buffer pointer and length may be adjusted past any prologue information

Arguments:

    lpBuffer            - pointer to pointer to buffer containing directory
                          listing

    lpdwBufferLength    - pointer to length of Buffer

    lpfnParserFunction  - returned directory parser function

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Bool,
                "DetermineDirectoryFormat",
                "%x [%.40q], %x [%d], %x",
                lpBuffer,
                *lpBuffer,
                lpdwBufferLength,
                *lpdwBufferLength,
                lpfnParserFunction
                ));

    BOOL success;

    if (!SkipWhitespace(lpBuffer, lpdwBufferLength)) {
        success = FALSE;
        goto quit;
    }

    if (IsNtDateFormat(*lpBuffer, *lpdwBufferLength)) {

        DEBUG_PRINT(FTP,
                    INFO,
                    ("format is NT\n"
                    ));

        *lpfnParserFunction = ParseNtDirectory;
        success = TRUE;
        goto quit;
    }

    //
    // we think the directory output is from Unix. The listing probably
    // starts with "total #" or a number, or other random garbage. We
    // know that a Unix dir listing starts with the ls attributes, so
    // we'll search for those, but keep our search within a reasonable
    // distance of the start
    //

    LPSTR buffer;
    DWORD length;
    char tokenBuffer[TOKEN_BUFFER_LENGTH];
    int lengthChecked;
    int iteration;
    int dataLength;

    buffer = *lpBuffer;
    length = *lpdwBufferLength;
    lengthChecked = 0;
    iteration = 0;
    dataLength = min((int)*lpdwBufferLength, RELATIVELY_SMALL_AMOUNT_OF_LS_DATA);

    while (lengthChecked < dataLength) {

        DWORD tokenLength;
        DWORD previousLength;

        tokenLength = sizeof(tokenBuffer);
        if (!GetToken(buffer,
                      length,
                      tokenBuffer,
                      &tokenLength)) {
            success = FALSE;
            goto quit;
        }
        lengthChecked += tokenLength;
        if (IsUnixAttributeFormat(tokenBuffer, tokenLength)) {

            DEBUG_PRINT(FTP,
                        INFO,
                        ("format is Unix\n"
                        ));

            *lpfnParserFunction = ParseUnixDirectory;
            *lpBuffer = buffer;
            *lpdwBufferLength = length;
            success = TRUE;
            goto quit;
        } else if ((iteration == 0)
        && (tokenLength == 5)
        && !strnicmp(tokenBuffer, "total", 5)) {

            //
            // there may be nothing in the directory listing, except
            // "total 0". If this is this case, then we recognize the
            // format
            //

            buffer += tokenLength;
            length -= tokenLength;
            tokenLength = sizeof(tokenBuffer) - 1;  // for '\0'
            if (!GetToken(buffer,
                          length,
                          tokenBuffer,
                          &tokenLength)) {
                success = FALSE;
                goto quit;
            }
            tokenBuffer[tokenLength] = '\0';
            if (isdigit(tokenBuffer[0]) && (atoi(tokenBuffer) == 0)) {

                DEBUG_PRINT(FTP,
                            INFO,
                            ("format is Unix - empty directory\n"
                            ));

                *lpfnParserFunction = ParseUnixDirectory;
                SkipLine(&buffer, &length);
                *lpBuffer = buffer;
                *lpdwBufferLength = length;
                success = TRUE;
                goto quit;
            }
        }

        //
        // try the next line
        //

        previousLength = length;
        SkipLine(&buffer, &length);
        lengthChecked += previousLength - length;
        ++iteration;
    }

    //
    // not NT or Unix. Lets try for OS/2. The format of an OS/2 directory entry
    // is:
    //
    //      [<ws>]<length>[DIR|<attribute>]<date><time><filename>
    //
    // we just try to parse the first line. If this succeeds, we assume OS/2
    // format
    //

    WIN32_FIND_DATA findData;
    PARSE_STATE state;

    buffer = *lpBuffer;
    length = *lpdwBufferLength;
    state = ParseOs2Directory(&buffer, &length, &findData);
    if ((state == State_Continue) || (state == State_Done)) {

        DEBUG_PRINT(FTP,
                    INFO,
                    ("format is OS/2\n"
                    ));

        success = TRUE;
        *lpfnParserFunction = ParseOs2Directory;
        goto quit;
    }

    //
    // Mac? Mac servers return Unix-like output which will (should) have already
    // been handled by the Unix listing check, and a very simple format which
    // just consists of names with an optional '/' appended, indicating a
    // directory
    //

    //
    // the Telnet 2.6 FTP server (which just reports the ultra-simple listing
    // format) returns a weird 'hidden' entry at the start of the listing which
    // consists of "\x03\x02\x01". We will skip all leading control characters
    //

    buffer = *lpBuffer;
    length = *lpdwBufferLength;
    while (length && (*buffer < ' ')) {
        ++buffer;
        --length;
    }

    LPSTR buffer_;
    DWORD length_;

    buffer_ = buffer;
    length_ = length;

    state = ParseMacDirectory(&buffer, &length, &findData);
    if ((state == State_Continue) || (state == State_Done)) {

        DEBUG_PRINT(FTP,
                    INFO,
                    ("format is Mac\n"
                    ));

        *lpBuffer = buffer_;
        *lpdwBufferLength = length_;
        success = TRUE;
        *lpfnParserFunction = ParseMacDirectory;
        goto quit;
    }

    //
    // failed to determine the format
    //

    success = FALSE;

quit:

    DEBUG_LEAVE(success);

    return success;
}


PRIVATE
BOOL
IsNtDateFormat(
    IN LPSTR lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Determines if the directory listing starts with an NT-style date field:

        MM-DD-YY

Arguments:

    lpBuffer        - pointer to buffer containing listing

    dwBufferLength  - number of bytes in lpBuffer

Return Value:

    BOOL
        TRUE    - buffer starts with NT-style date format

        FALSE   - not NT-listing

--*/

{
    if (dwBufferLength > 8) {

        LPSTR buffer;
        WORD number;

        buffer = lpBuffer;
        if (!ParseWord(&buffer, &dwBufferLength, 1, 12, &number)) {
            return FALSE;
        }
        if (!((dwBufferLength > 0) && (*buffer == '-'))) {
            return FALSE;
        }
        ++buffer;
        --dwBufferLength;
        if (!ParseWord(&buffer, &dwBufferLength, 1, 31, &number)) {
            return FALSE;
        }
        if (!((dwBufferLength > 0) && (*buffer == '-'))) {
            return FALSE;
        }
        ++buffer;
        --dwBufferLength;
        return ParseWord(&buffer, &dwBufferLength, 0, MAX_YEAR_SUPPORTED, &number);
    }
    return FALSE;
}


PRIVATE
BOOL
GetToken(
    IN LPSTR lpszBuffer,
    IN DWORD dwBufferLength,
    OUT LPSTR lpszToken,
    IN OUT LPDWORD lpdwTokenLength
    )

/*++

Routine Description:

    Copies a token out of the buffer without updating the buffer pointer or length

Arguments:

    lpszBuffer      - pointer to buffer to copy from

    lpBufferLength  - length of buffer

    lpszToken       - buffer to copy to

    lpdwTokenLength - length of buffer on input, length of token on output

Return Value:

    BOOL

--*/

{
    DWORD length;

    if (!SkipSpaces(&lpszBuffer, &dwBufferLength)) {
        return FALSE;
    }

    if (dwBufferLength == 0) {
        return FALSE;
    }

    length = *lpdwTokenLength;
    while (!isspace(*lpszBuffer) && (dwBufferLength != 0) && (length != 0)) {
        *lpszToken++ = *lpszBuffer++;
        --dwBufferLength;
        --length;
    }
    *lpdwTokenLength -= length;
    return TRUE;
}


PRIVATE
BOOL
IsUnixAttributeFormat(
    IN LPSTR lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Checks if the buffer contains a Unix ls attribute field format

Arguments:

    lpBuffer        - pointer to buffer containing token to check

    dwBufferLength  - length of buffer

Return Value:

    BOOL
        TRUE    - lpBuffer *probably* contained a Unix attribute field
        FALSE   - lpBuffer *probably didn't* contain a Unix attribute field

--*/

{
    int i;
    int hits;

    if (dwBufferLength != 10) {
        return FALSE;
    }

    //
    // the first character contains 'd' for directory, 'l' for link, '-' for
    // file, and may contain other, unspecified characters, so we just ignore
    // it. So long as the next 9 characters are in the set [-rwx] then we have
    // a Unix ls attribute field.
    //
    // N.B. it turns out that the first character can be in the set [-bcdlp]
    // and the attribute characters can be in the set [-lrsStTwx] (as of
    // 08/18/95)
    //

    ++lpBuffer;
    hits = 0;
    for (i = 0; i < 9; ++i) {

        char ch;

        ch = tolower(*lpBuffer);
        ++lpBuffer;

        if ((ch == '-')
        || (ch == 'l')
        || (ch == 'r')
        || (ch == 's')
        || (ch == 't')
        || (ch == 'w')
        || (ch == 'x')) {
            ++hits;
        }
    }

    //
    // new scheme: we decide if the token was a Unix attribute field based on
    // probability. If the hit rate was greater than 1 in 2 (5 out of 9 or
    // higher) then we say that the field was probably a Unix attribute. This
    // scheme allows us to accept future enhancements or non-standard Unix
    // implementations without changing this code (for a while) (Make the
    // attribute set a registry value (!))
    //

    return hits >= 5;
}


PRIVATE
PARSE_STATE
ParseNtDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Parses a single line of an NT directory listing (output from DIR) into a
    WIN32_FIND_DATA structure

    The format of an NT directory list line is:

    <date> <time> <'<DIR>'|<size>> <filename>

Arguments:

    lpBuffer        - pointer to pointer to directory listing

    lpBufferLength  - pointer to number of bytes remaining in lpBuffer

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    PARSE_STATE
        State_Continue  - more listing to parse

        State_Done      - fin!

--*/

{
    //
    // not expecting the line to start with spaces, but we check anyway
    //

    if (!SkipSpaces(lpBuffer, lpBufferLength)) {
        goto done;
    }

    if (!ExtractNtDate(lpBuffer, lpBufferLength, lpFindData)) {
        goto done;
    }

    if (!strnicmp(*lpBuffer, "<DIR>", 5)) {
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        lpFindData->nFileSizeHigh = 0;
        lpFindData->nFileSizeLow = 0;
        FindTokenSpecial(lpBuffer, lpBufferLength);
    } else {
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        ExtractFileSize(lpBuffer, lpBufferLength, lpFindData);
        SkipSpaces(lpBuffer, lpBufferLength);
    }

    _ExtractFilename(lpBuffer, lpBufferLength, lpFindData);

    //
    // we expect the filename to be the last thing on the line
    //

done:

    return SkipLine(lpBuffer, lpBufferLength) ? State_Continue : State_Done;
}


PRIVATE
void
ReadUnixPermissions(
    IN LPCSTR pszBuffer,
    IN DWORD cbBufferSize,
    OUT LPDWORD pdwPermissions)
{
    // Format: rwxrwxrwx <Owner><Group><All>
    *pdwPermissions = 0;

    if (cbBufferSize > 10)
    {
        if ('r' == pszBuffer[1])
            *pdwPermissions |= 0x00000400;

        if ('w' == pszBuffer[2])
            *pdwPermissions |= 0x00000200;

        if ('x' == pszBuffer[3])
            *pdwPermissions |= 0x00000100;

        if ('r' == pszBuffer[4])
            *pdwPermissions |= 0x00000040;

        if ('w' == pszBuffer[5])
            *pdwPermissions |= 0x00000020;

        if ('x' == pszBuffer[6])
            *pdwPermissions |= 0x00000010;

        if ('r' == pszBuffer[7])
            *pdwPermissions |= 0x00000004;

        if ('w' == pszBuffer[8])
            *pdwPermissions |= 0x00000002;

        if ('x' == pszBuffer[9])
            *pdwPermissions |= 0x00000001;
    }
}


PRIVATE
PARSE_STATE
ParseUnixDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Parses a single line of a Unix directory listing (output from ls) into a
    WIN32_FIND_DATA structure

    The format of a Unix directory list line is:

    <attributes> <link-count> <owner> <group> <size> <date-time> <filename>

Arguments:

    lpBuffer        - pointer to pointer to directory listing

    lpBufferLength  - pointer to number of bytes remaining in lpBuffer

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    PARSE_STATE
        State_Continue  - more listing to parse

        State_Done      - fin!

--*/

{
    DWORD error;
    int i;
    BOOL symbolicLink;
    char ch;

    //
    // not expecting the line to start with spaces, but we check anyway
    //

    if (!SkipSpaces(lpBuffer, lpBufferLength)) {
        goto done;
    }

    //
    // if the item is a symbolic link then we have to trim the 'filename' below
    //

    ch = tolower(**lpBuffer);
    symbolicLink = (ch == 'l');

    //
    // attributes are first thing on line
    //

    lpFindData->dwFileAttributes = (ch == 'd') ? FILE_ATTRIBUTE_DIRECTORY
                                 : (ch == '-') ? FILE_ATTRIBUTE_NORMAL
                                 : 0;

    //
    // skip over the attributes and over the owner/creator fields to the file
    // size
    //
    
    // Read the Attributes and put them in the WIN32_FIND_DATA.dwReserved0 attributes.
    // It's OK to use FILE_ATTRIBUTE_REPARSE_POINT because it's unused unless
    // WIN32_FIND_DATA.dwFileAttributes contains yyy, which we don't set.
    ReadUnixPermissions(*lpBuffer, *lpBufferLength, &(lpFindData->dwReserved0));

    LPSTR lpszLastToken;
    DWORD dwLastToken;

    for (i = 0; i < 4; ++i) {
        lpszLastToken = *lpBuffer;
        dwLastToken = *lpBufferLength;
        if (!FindToken(lpBuffer, lpBufferLength)) {
            goto done;
        }
    }

    if (!ExtractFileSize(lpBuffer, lpBufferLength, lpFindData)) {
        ExtractFileSize(&lpszLastToken, &dwLastToken, lpFindData);
    }

    SkipSpaces(lpBuffer, lpBufferLength);

    if (!ExtractUnixDate(lpBuffer, lpBufferLength, lpFindData)) {
        goto done;
    }

    SkipSpaces(lpBuffer, lpBufferLength);

    //
    // we expect the filename to be the last thing on the line
    //

    _ExtractFilename(lpBuffer, lpBufferLength, lpFindData);

    //
    // if the item is a symbolic link, then remove everything after the " -> "
    //

    if (symbolicLink) {

        LPSTR lpArrow;

        lpArrow = strstr(lpFindData->cFileName, " -> ");
        if (lpArrow != NULL) {
            *lpArrow = '\0';
        }
    }

done:

    return SkipLine(lpBuffer, lpBufferLength) ? State_Continue : State_Done;
}


PRIVATE
PARSE_STATE
ParseOs2Directory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Parses a single line of an OS/2 directory listing (output from dir) into a
    WIN32_FIND_DATA structure

    The format of an OS/2 directory list line is:

    <size> <attributes> <date> <time> <filename>

    This function is also used to determine OS/2 directory format

Arguments:

    lpBuffer        - pointer to pointer to directory listing

    lpBufferLength  - pointer to number of bytes remaining in lpBuffer

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    PARSE_STATE
        State_Continue  - more listing to parse

        State_Done      - fin!

        State_Error     - directory format not recognized
--*/

{
    PARSE_STATE state;

    if (!SkipSpaces(lpBuffer, lpBufferLength)) {
        goto skip;
    }

    if (!ExtractFileSize(lpBuffer, lpBufferLength, lpFindData)) {
        state = State_Error;
        goto done;
    }

    if (!SkipSpaces(lpBuffer, lpBufferLength)) {
        goto skip;
    }

    if (!strnicmp(*lpBuffer, "DIR", 3)) {
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        FindToken(lpBuffer, lpBufferLength);
    } else if (!isdigit(**lpBuffer)) {
        if (!ExtractOs2Attributes(lpBuffer, lpBufferLength, lpFindData)) {
            state = State_Error;
            goto done;
        }
        SkipSpaces(lpBuffer, lpBufferLength);
    } else {
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    if (!ExtractNtDate(lpBuffer, lpBufferLength, lpFindData)) {
        state = State_Error;
        goto done;
    }

    _ExtractFilename(lpBuffer, lpBufferLength, lpFindData);

    //
    // we expect the filename to be the last thing on the line
    //

skip:

    state = SkipLine(lpBuffer, lpBufferLength) ? State_Continue : State_Done;

done:

    return state;
}


PRIVATE
PARSE_STATE
ParseMacDirectory(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Parses a single line of a Mac directory listing (output from ls) into a
    WIN32_FIND_DATA structure

    The format of a Mac directory list line is:

    <dir-or-file-name>[/]

    This function is also used to determine Mac directory format

Arguments:

    lpBuffer        - pointer to pointer to directory listing

    lpBufferLength  - pointer to number of bytes remaining in lpBuffer

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    PARSE_STATE
        State_Continue  - more listing to parse

        State_Done      - fin!

        State_Error     - directory format not recognized

--*/

{
    PARSE_STATE state;

    if (!SkipSpaces(lpBuffer, lpBufferLength)) {
        goto skip;
    }

    _ExtractFilename(lpBuffer, lpBufferLength, lpFindData);

    int len;

    len = lstrlen(lpFindData->cFileName);
    if (lpFindData->cFileName[len - 1] == '/') {
        lpFindData->cFileName[len - 1] = '\0';
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    } else {
        lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }
    if ((*lpBufferLength != 0)
    && !((**lpBuffer == '\r') || (**lpBuffer == '\n'))) {
        state = State_Error;
        goto done;
    }

    //
    // this directory format has no size or time information
    //

    lpFindData->nFileSizeLow = 0;
    lpFindData->nFileSizeHigh = 0;
    SystemTimeToFileTime(&DefaultSystemTime, &lpFindData->ftLastWriteTime);

    //
    // we expect the filename to be the last thing on the line
    //

skip:

    state = SkipLine(lpBuffer, lpBufferLength) ? State_Continue : State_Done;

done:

    return state;
}


PRIVATE
BOOL
ExtractFileSize(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Extracts the the next token in the directory listing. The next token is
    expected to be the file size. It is extracted to the WIN32_FIND_DATA
    structure

    Assumes:    1. The file size is <= 32 bits

Arguments:

    lpBuffer        - pointer to pointer to directory listing buffer

    lpBufferLength  - pointer to remaining buffer length

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    BOOL

--*/

{
    INET_ASSERT(*lpBufferLength != 0);

    LPSTR buffer;
    char ch = **lpBuffer;

    if (isdigit(ch)) {
        lpFindData->nFileSizeLow = strtoul(*lpBuffer, &buffer, 10);
        lpFindData->nFileSizeHigh = 0;
        *lpBufferLength -= (DWORD) (buffer - *lpBuffer);
        *lpBuffer = buffer;
        return TRUE;
    }
    return FALSE;
}


PRIVATE
BOOL
_ExtractFilename(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Extracts the filename from the current directory listing position into the
    WIN32_FIND_DATA structure

Arguments:

    lpBuffer        - pointer to pointer to directory listing buffer

    lpBufferLength  - pointer to remaining buffer length

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    BOOL

--*/

{
    LPSTR dest;
    DWORD destLength;

    dest = lpFindData->cFileName;
    destLength = sizeof(lpFindData->cFileName) - 1;
    while ((*lpBufferLength != 0)
    && (destLength != 0)
    && !((**lpBuffer == '\r') || (**lpBuffer == '\n'))) {
        *dest++ = *(*lpBuffer)++;
        --*lpBufferLength;
        --destLength;
    }
    *dest = '\0';
    return TRUE;
}


PRIVATE
BOOL
ExtractNtDate(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Extracts an NT date and time from the directory listing. NT dates have a
    specific format:

        MM-DD-YY hh:mmPP

Arguments:

    lpBuffer        - pointer to pointer to current position in directory list

    lpBufferLength  - number of bytes til end of directory list

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    SYSTEMTIME systemTime;
    LPSTR buffer;
    DWORD buflen;
    int number;
    LPSTR stop;
    LPSYSTEMTIME lpSystemTime;

    lpSystemTime = &DefaultSystemTime;

    //
    // BUGBUG - what about internationalization? E.g. does UK FTP server return
    //          the date as e.g. 27/07/95? Other formats?
    //

    //
    // month ::= 1..12
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 1, 12, &systemTime.wMonth)) {
        goto done;
    }
    if (!((*lpBufferLength > 0) && (**lpBuffer == '-'))) {
        goto done;
    }
    ++*lpBuffer;
    --*lpBufferLength;

    //
    // date ::= 1..31
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 1, 31, &systemTime.wDay)) {
        goto done;
    }
    if (!((*lpBufferLength > 0) && (**lpBuffer == '-'))) {
        goto done;
    }
    ++*lpBuffer;
    --*lpBufferLength;

    //
    // year ::= 0..2100
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 0, MAX_YEAR_SUPPORTED, &systemTime.wYear)) {
        goto done;
    }
    if (!((*lpBufferLength > 0) && (**lpBuffer == ' '))) {
        goto done;
    }

    //
    // the oldest file can be dated 1980. We allow the following:
    //
    //  1-1-79              => 1-1-2079
    //  1-1-80..12-31-99    => 1-1-1980..12-31-1999
    //  1-1-00              => 1-1-2000
    //  1-1-1995            => 1-1-1995
    //  1-1-2001            => 1-1-2001
    //  etc.
    //

    systemTime.wYear += (systemTime.wYear < 80)
                        ? 2000
                        : (systemTime.wYear <= 99)
                            ? 1900
                            : 0
                            ;

    //
    // find start of time (er, Professor Hawking..?)
    //

    if (!FindToken(lpBuffer, lpBufferLength)) {
        goto done;
    }

    //
    // hour ::= 0..23 | 1..12
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 0, 23, &systemTime.wHour)) {
        goto done;
    }
    if (!((*lpBufferLength > 0) && (**lpBuffer == ':'))) {
        goto done;
    }
    ++*lpBuffer;
    --*lpBufferLength;

    //
    // minute ::= 0..59
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 0, 59, &systemTime.wMinute)) {
        goto done;
    }

    //
    // if the time is followed by AM or PM then convert to 24-hour time if PM
    // and skip to end of token in both cases
    //

    if (*lpBufferLength >= 2) {

        char ch_p;

        ch_p = tolower(**lpBuffer);
        if ((ch_p == 'p') || (ch_p == 'a')) {

            char ch_m;

            ch_m = tolower(*(*lpBuffer + 1));
            if ((ch_p == 'p') && (ch_m == 'm')) {
                // 12 PM = 12, 1PM = 13, 2PM = 14, etc
                if ( systemTime.wHour < 12 ) {
                    systemTime.wHour += 12;
                }
            } else if ( systemTime.wHour == 12 ) {
                // 12 AM == 0:00 24hr
                INET_ASSERT((ch_p == 'a') && (ch_m == 'm'));
                systemTime.wHour = 0;
            }
        }
    }

    //
    // seconds, milliseconds and weekday always zero
    //

    systemTime.wSecond = 0;
    systemTime.wMilliseconds = 0;
    systemTime.wDayOfWeek = 0;

    //
    // get ready to convert the parsed date/time to a FILETIME
    //

    lpSystemTime = &systemTime;

done:

    //
    // convert the system time to file time and move the buffer pointer/length
    // to the next line
    //

    if (!SystemTimeToFileTime(lpSystemTime, &lpFindData->ftLastWriteTime)) {
        SystemTimeToFileTime(&DefaultSystemTime, &lpFindData->ftLastWriteTime);
    }
    FindToken(lpBuffer, lpBufferLength);
    return TRUE;
}


PRIVATE
BOOL
ExtractUnixDate(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Extracts a Unix date and time from the directory listing. Unix dates have a
    multitude of formats:

        Jul  3 14:52
        Oct  7 1994
        Jul 26  4:05
        Jul 26 03:51

Arguments:

    lpBuffer        - pointer to pointer to current position in directory list

    lpBufferLength  - number of bytes til end of directory list

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    SYSTEMTIME systemTime;
    LPSYSTEMTIME lpSystemTime;
    static LPSTR Months = "janfebmaraprmayjunjulaugsepoctnovdec";
    char monthStr[4];
    int i;
    LPSTR offset;
    WORD wnum;

    lpSystemTime = &DefaultSystemTime;

    //
    // month ::= Jan..Dec
    //

    for (i = 0; i < 3; ++i) {
        if (*lpBufferLength == 0) {
            goto done;
        }
        monthStr[i] = *(*lpBuffer)++;
        monthStr[i] = tolower(monthStr[i]);
        --*lpBufferLength;
    }
    monthStr[i] = '\0';
    offset = strstr(Months, monthStr);
    if (offset == NULL) {
        goto done;
    }
    systemTime.wMonth = (unsigned short) ((offset-Months) / 3 + 1);

    FindToken(lpBuffer, lpBufferLength);

    //
    // date ::= 1..31
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 1, 31, &systemTime.wDay)) {
        goto done;
    }

    SkipSpaces(lpBuffer, lpBufferLength);

    //
    // year or hour
    //

    if (!ParseWord(lpBuffer, lpBufferLength, 0, 65535, &wnum)) {
        goto done;
    }

    if ((*lpBufferLength != 0) && (**lpBuffer == ':')) {

        SYSTEMTIME timeNow;

        systemTime.wHour = wnum;

        //
        // we found the hour field, now get the minutes
        //

        ++*lpBuffer;
        --*lpBufferLength;
        if (!ParseWord(lpBuffer, lpBufferLength, 0, 59, &systemTime.wMinute)) {
            goto done;
        }

        //
        // a date-time with an hour:minute field is based in this year. We need
        // to get the current year from the system. There is a slight problem in
        // that if this machine's just had a new year and the FTP server is
        // behind us, then the file can be a year out of date.
        //
        // There is no guarantees about the basis of time used by the FTP server,
        // so we'll get the UTC (aka Greenwich Mean Time) time
        //

        GetSystemTime(&timeNow);

        //
        // apparently its not quite as straightforward as first we'd believed.
        // If the date/month is in the future then the year is last year
        //

        BOOL bLastYear = FALSE;

        if (systemTime.wMonth > timeNow.wMonth) {
            bLastYear = TRUE;
        } else if (systemTime.wMonth == timeNow.wMonth) {

            //
            // BUGBUG - leap year? I believe that in this case, because the time
            //          difference is 1 year minus 1 day, then that is great
            //          enough for the date format including year to have been
            //          used and thus making this moot. Need to prove it.
            //          Note, by that logic, everything from here on down should
            //          also be moot - we should only have to concern ourselves
            //          with the month
            //

            if (systemTime.wDay > timeNow.wDay) {
                bLastYear = TRUE;
            } else if (systemTime.wDay == timeNow.wDay) {
                if (systemTime.wHour > timeNow.wHour) {
                    bLastYear = TRUE;
                } else if (systemTime.wHour == timeNow.wHour) {
                    if (systemTime.wMinute > timeNow.wMinute) {
                        bLastYear = TRUE;
                    } else if (systemTime.wMinute == timeNow.wMinute) {
                        if (systemTime.wSecond > timeNow.wSecond) {
                            bLastYear = TRUE;
                        }
                    }
                }
            }
        }
        systemTime.wYear = timeNow.wYear;
        if (bLastYear) {
            --systemTime.wYear;
        }
    } else {

        //
        // next field is the year
        //

        systemTime.wYear = wnum;

        //
        // time for a file with only a year is 00:00 (mitternacht)
        //

        systemTime.wHour = 0;
        systemTime.wMinute = 0;
    }

    //
    // seconds, milliseconds and weekday always zero
    //

    systemTime.wSecond = 0;
    systemTime.wMilliseconds = 0;
    systemTime.wDayOfWeek = 0;

    //
    // get ready to convert the parsed date/time to a FILETIME
    //

    lpSystemTime = &systemTime;

done:

    //
    // convert the system time to file time and move the buffer pointer/length
    // to the next line
    //

    if (!SystemTimeToFileTime(lpSystemTime, &lpFindData->ftLastWriteTime)) {
        SystemTimeToFileTime(&DefaultSystemTime, &lpFindData->ftLastWriteTime);
    }
    FindTokenSpecial(lpBuffer, lpBufferLength);
    return TRUE;
}


PRIVATE
BOOL
ExtractOs2Attributes(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN OUT LPWIN32_FIND_DATA lpFindData
    )

/*++

Routine Description:

    Converts an OS/2 attribute string into Win32 file attribute flags in the
    WIN32_FIND_DATA structure

Arguments:

    lpBuffer        - pointer to pointer to current position in directory list

    lpBufferLength  - number of bytes til end of directory list

    lpFindData      - pointer to WIN32_FIND_DATA to update

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    DWORD attributes = 0;
    BOOL done = FALSE;

    while (*lpBufferLength && !done) {

        char ch = **lpBuffer;

        switch (toupper(ch)) {
        case 'A':
            attributes |= FILE_ATTRIBUTE_ARCHIVE;
            break;

        case 'H':
            attributes |= FILE_ATTRIBUTE_HIDDEN;
            break;

        case 'R':
            attributes |= FILE_ATTRIBUTE_READONLY;
            break;

        case 'S':
            attributes |= FILE_ATTRIBUTE_SYSTEM;
            break;

        case ' ':

            //
            // if there is only one space, we will be pointing at the next token
            // after this function completes. Okay so long as we will be calling
            // SkipSpaces() next (which doesn't expect to be pointing at a space)
            //

            done = TRUE;
            break;
        }
        --*lpBufferLength;
        ++*lpBuffer;
    }

    //
    // if we are here there must have been some characters which looked like
    // OS/2 file attributes
    //

    INET_ASSERT(attributes != 0);

    lpFindData->dwFileAttributes = attributes;

    return TRUE;
}


PRIVATE
BOOL
ParseWord(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    IN WORD LowerBound,
    IN WORD UpperBound,
    OUT LPWORD lpNumber
    )

/*++

Routine Description:

    Extract a WORD value out of the buffer. To be correctly parsed the number
    field must:

        * start with numeric characters
        * not be negative
        * be >= LowerBound and <= UpperBound
        * must end with a non-numeric character or when the buffer is exhausted

Arguments:

    lpBuffer        - pointer to pointer to buffer containing number to parse

    lpBufferLength  - pointer to remaining length in buffer

    LowerBound      - lowest value converted number can have

    UpperBound      - highest value converted number can have

    lpNumber        - pointer to returned WORD value

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    int number;

    if (ExtractInteger(lpBuffer, lpBufferLength, &number)) {
        if ((number >= (int)LowerBound) && (number <= (int)UpperBound)) {
            *lpNumber = (WORD)number;
            return TRUE;
        }
    }
    return FALSE;
}


PRIVATE
BOOL
ExtractInteger(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength,
    OUT LPINT lpNumber
    )

/*++

Routine Description:

    Performs a strtoul type function, but the input string is not terminated by
    \0. It has an associated length

Arguments:

    lpBuffer        - pointer to pointer to string containing number to extract

    lpBufferLength  - pointer to length of string remaining in lpBuffer

    lpNumber        - pointer to returned value

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    BOOL success;
    int number;

    number = 0;
    if ((*lpBufferLength > 0) && isdigit(**lpBuffer)) {
        while (isdigit(**lpBuffer) && (*lpBufferLength != 0)) {
            number = number * 10 + (int)(**lpBuffer - '0');
            ++*lpBuffer;
            --*lpBufferLength;
        }
        success = TRUE;
    } else {
        success = FALSE;
    }
    *lpNumber = number;
    return success;
}
