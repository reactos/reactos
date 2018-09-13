/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    parse.cxx

Abstract:

    Contains functions to parse gopher information received from the server

    Contents:
        IsValidLocator
        IsGopherPlus
        CrackLocator
        GopherCharToType
        GopherTypeToChar
        GetDirEntry
        (GopherLocatorToFindData)
        ReadData
        (ExtractLine)
        (ExtractDisplayString)
        CopyToEol
        IsGopherPlusToken
        MapAttributeNameToId
        MapAttributeToIds
        MapAttributeIdToNames
        GetGopherNumber
        ExtractDateAndTime
        ExtractView
        FindAttribute
        FindNextAttribute
        EnumerateAttribute
        ParseIntField
        ParseDwordField
        ParseStringField
        ParseAdminAttribute
        ParseModDateAttribute
        ParseAbstractAttribute
        ParseViewAttribute
        ParseTreewalkAttribute
        ParseUnknownAttribute
        (ExtractAttributeName)
        (CharacterCount)
        (CountCharactersToEol)
        (CopyString)

Author:

    Richard L Firth (rfirth) 17-Oct-1994

Environment:

    Win32 user-level DLL

Revision History:

    17-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"
#include "iert.h"

//
// manifests
//

#define DEFAULT_LINE_BUFFER_LENGTH          1024    // arbitrary
#define DEFAULT_ATTRIBUTE_NAME_LENGTH       64      // "
#define DEFAULT_LANGUAGE_NAME_LENGTH        32      // "
#define DEFAULT_CONTENT_TYPE_NAME_LENGTH    80      // "
#define SEARCH_TYPE_MOD_DATE                1
#define SEARCH_TYPE_VIEW                    2

//
// macros
//

#define NUMERIC_CHARACTER_TO_NUMBER(c)  ((int)(c) - (int)('0'))

//
// prototypes
//

PRIVATE
DWORD
GopherLocatorToFindData(
    IN LPCSTR Locator,
    IN DWORD Length,
    OUT LPGOPHER_FIND_DATA FindData
    );

PRIVATE
DWORD
ExtractLine(
    IN LPVIEW_INFO ViewInfo,
    OUT LPBYTE LineBuffer,
    IN OUT LPDWORD LineBufferLength,
    IN OUT LPDWORD DataOffset
    );

PRIVATE
DWORD
ExtractDisplayString(
    IN LPCSTR Locator,
    IN OUT LPSTR* StringPointer,
    IN DWORD BufferLength
    );

PRIVATE
BOOL
SkipLeading(
    IN OUT LPSTR* String,
    IN OUT LPDWORD Length
    );

PRIVATE
DWORD
ExtractAttributeName(
    OUT LPSTR AttributeName,
    IN OUT LPDWORD AttributeNameLength,
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength
    );

PRIVATE
DWORD
CharacterCount(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    IN LPSTR TerminationSet
    );

PRIVATE
DWORD
CountCharactersToEol(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength
    );

PRIVATE
VOID
CopyString(
    IN OUT LPSTR* String,
    IN LPSTR Source,
    IN DWORD Length
    );

//
// functions
//


BOOL
IsValidLocator(
    IN LPCSTR Locator,
    IN DWORD MaximumLength
    )

/*++

Routine Description:

    Given a locator string, determines whether it is a valid gopher locator. A
    valid gopher locator must have the form:

    <GopherChar><DisplayString>TAB<SelectorString>TAB<HostName>TAB<Port>[TAB<Gopher+Stuff>]<CR><LF><EOS>

    We don't care about the contents of DisplayString, SelectorString, HostName,
    Port or Gopher+Stuff, since these will be sorted out by sockets functions or
    the gopher protocol

Arguments:

    Locator         - pointer to locator string

    MaximumLength   - maximum number characters that can be in the locator

Return Value:

    BOOL
        TRUE    - Locator is valid

        FALSE   - Locator does not look like kosher gopher locator, already

--*/

{
    BOOL success;

    success = FALSE;
    __try {

        DWORD locatorLength;

        locatorLength = strlen(Locator);

        //
        // 1. Since there are all sorts of unspecified gopher types in the world,
        // we no longer test the type, but just make sure its not 0 (which would
        // have yielded a zero locatorLength). Also check that the locator doesn't
        // break the maximum length limit
        //

        if ((locatorLength != 0) && (locatorLength <= MaximumLength)) {

            //
            // 2. <DisplayString>. Can be empty. This can be any character, ANSI
            // or otherwise, we just don't care about its contents
            //

            ++Locator;
            --locatorLength;
            while ((*Locator != '\t') && (locatorLength != 0)) {
                --locatorLength;
                ++Locator;
            }

            if ((*Locator == '\t') && (locatorLength != 0)) {

                //
                // 3. <SelectorString>. Same rules as for DisplayString: contents
                // not interesting
                //

                ++Locator;
                --locatorLength;
                while ((*Locator != '\t') && (locatorLength != 0)) {
                    --locatorLength;
                    ++Locator;
                }

                if ((*Locator == '\t') && (locatorLength != 0)) {

                    //
                    // 4. <HostName>. Again, we don't care about the characters
                    // that comprise HostName, or the length. We used to require
                    // a non-zero length
                    //

                    ++Locator;
                    --locatorLength;
                    while ((*Locator != '\t') && (locatorLength != 0)) {
                        --locatorLength;
                        ++Locator;
                    }

                    if ((*Locator == '\t') && (locatorLength != 0)) {

                        DWORD number;

                        //
                        // 5. Port. This must comprise 0..5 digit characters
                        //

                        ++Locator;
                        --locatorLength;
                        number = 0;
                        while ((*Locator != '\t')
                        && (*Locator != '\r')
                        && (*Locator != '\n')
                        && (*Locator >= '0')
                        && (*Locator <= '9')) {

                            //
                            // we are kind of assuming no leading zeroes...
                            //

                            number = number * 10 + (DWORD)(*Locator - '0');
                            --locatorLength;
                            ++Locator;
                        }

                        if (number <= (DWORD)INTERNET_MAX_PORT_NUMBER_VALUE) {

                            //
                            // 6. Optional gopher+ characters. We ignore the
                            // rest of the locator, and assume that it is
                            // correct
                            //

                            if ((*Locator == '\t') && (locatorLength >= 2)) {
                                do {
                                    ++Locator;
                                    --locatorLength;
                                } while (  (*Locator != '\r')
                                        && (*Locator != '\n')
                                        && (locatorLength != 0) );
                            }

                            //
                            // check for line termination. Because of the random
                            // nature of gopher servers, we allow 0 or more '\r'
                            // followed by '\n'. The locator MUST be terminated
                            // by '\n'
                            //

                            while ((*Locator == '\r') && (locatorLength != 0)) {
                                ++Locator;
                                --locatorLength;
                            }
                            if ((*Locator == '\n') && (locatorLength == 1)) {
                                success = TRUE;
                            }
                        }
                    }
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        success = FALSE;
    }
    ENDEXCEPT
    return success;
}


BOOL
IsGopherPlus(
    IN LPCSTR Locator
    )

/*++

Routine Description:

    Returns TRUE if Locator describes a gopher+ request

    ASSUMES:    1. Locator is valid

Arguments:

    Locator - pointer to locator to check

Return Value:

    BOOL

--*/

{
    LPSTR plusStuff;

    if (!IsValidLocator(Locator, strlen(Locator))) {
        return FALSE;
    }

    //
    // use CrackLocator to see if there is gopher+ info on this locator
    //

    CrackLocator(Locator,
                 NULL,  // Type
                 NULL,  // DisplayString
                 NULL,  // DisplayStringLength
                 NULL,  // SelectorString
                 NULL,  // SelectorStringLength
                 NULL,  // HostName
                 NULL,  // HostNameLength
                 NULL,  // GopherPort
                 &plusStuff
                 );
    return (BOOL)(plusStuff != NULL);
}


BOOL
CrackLocator(
    IN LPCSTR Locator,
    OUT LPDWORD Type OPTIONAL,
    OUT LPSTR DisplayString OPTIONAL,
    IN OUT LPDWORD DisplayStringLength OPTIONAL,
    OUT LPSTR SelectorString OPTIONAL,
    IN OUT LPDWORD SelectorStringLength OPTIONAL,
    OUT LPSTR HostName OPTIONAL,
    IN OUT LPDWORD HostNameLength OPTIONAL,
    OUT LPDWORD GopherPort OPTIONAL,
    OUT LPSTR* ExtraStuff OPTIONAL
    )

/*++

Routine Description:

    Given a locator, break it into its constituent parts. The Locator argument
    is NOT modified

    ASSUMES:    1. Locator is valid

                2. If an optional pointer is supplied, the associated length
                   parameter (if applicable) must also be supplied

Arguments:

    Locator                 - pointer to locator to crack

    Type                    - optional returned type character

    DisplayString           - optional returned display string

    DisplayStringLength     - optional in/out display string buffer length

    SelectorString          - optional returned selector string

    SelectorStringLength    - optional in/out selector string buffer length

    HostName                - optional returned host name

    HostNameLength          - optional in/out host name buffer length

    GopherPort              - optional returned gopher port

    ExtraStuff              - optional returned extra (gopher+) data from end of locator.
                              This argument is a returned pointer, not a buffer. Care
                              should be taken since this argument aliases Locator (or
                              part thereof)

Return Value:

    TRUE    - locator cracked ok

    FALSE   - problem encountered cracking locator, probably substring
              breaks buffer limit

--*/

{
    LPSTR pTab;
    DWORD len;
    LPSTR extraStuff;
    DWORD locatorLength;

    locatorLength = strlen(Locator);
    if (ARGUMENT_PRESENT(Type)) {
        *Type = GopherCharToType(*Locator);
    }

    ++Locator;  // past type character
    --locatorLength;
    pTab = (LPSTR)memchr((LPVOID)Locator, '\t', locatorLength);

    INET_ASSERT(pTab != NULL);

    len = (DWORD) (pTab - Locator);
    if (ARGUMENT_PRESENT(DisplayString)) {

        INET_ASSERT(DisplayStringLength != NULL);

        if (*DisplayStringLength <= len) {
            return FALSE;
        }
        memcpy(DisplayString, Locator, len);
        DisplayString[len] = '\0';
        *DisplayStringLength = len;
    }

    Locator = pTab + 1; // past display string and TAB
    locatorLength -= (len + 1);

    pTab = (LPSTR)memchr((LPVOID)Locator, '\t', locatorLength);

    INET_ASSERT(pTab != NULL);

    len = (DWORD) (pTab - Locator);
    if (ARGUMENT_PRESENT(SelectorString)) {

        INET_ASSERT(SelectorStringLength != NULL);

        if (*SelectorStringLength <= len) {
            return FALSE;
        }
        memcpy(SelectorString, Locator, len);
        SelectorString[len] = '\0';
        *SelectorStringLength = len;
    }

    Locator = pTab + 1; // past selector string and TAB
    locatorLength -= (len + 1);

    pTab = (LPSTR)memchr((LPVOID)Locator, '\t', locatorLength);

    INET_ASSERT(pTab != NULL);

    len = (DWORD) (pTab - Locator);
    if (ARGUMENT_PRESENT(HostName)) {

        INET_ASSERT(HostNameLength != NULL);

        if (*HostNameLength <= len) {
            return FALSE;
        }
        memcpy(HostName, Locator, len);
        HostName[len] = '\0';
        *HostNameLength = len;
    }

    Locator = pTab + 1; // past host name and TAB
    locatorLength -= (len + 1);
    pTab = (LPSTR)memchr(Locator, '\t', locatorLength);
    if (pTab != NULL) {
        extraStuff = pTab + 1;  // past port and TAB
    } else {
        extraStuff = NULL;
    }

    if (ARGUMENT_PRESENT(GopherPort)) {
        *GopherPort = (DWORD)STRTOUL(Locator, NULL, 10);
    }

    if (ARGUMENT_PRESENT(ExtraStuff)) {
        *ExtraStuff = extraStuff;
    }

    return TRUE;
}


DWORD
GopherCharToType(
    IN CHAR GopherChar
    )

/*++

Routine Description:

    Converts the gopher descriptor character to a Gfr attribute

Arguments:

    GopherChar  - the gopher character to convert

Return Value:

    DWORD
        mapped gopher type or GOPHER_TYPE_UNKNOWN if we don't recognise the
        character

--*/

{
    //
    // these are the types currently specified in RFC 1436 (plus a few that
    // aren't)
    //

    switch (GopherChar) {
    case GOPHER_CHAR_TEXT_FILE:
        return GOPHER_TYPE_TEXT_FILE;

    case GOPHER_CHAR_DIRECTORY:
        return GOPHER_TYPE_DIRECTORY;

    case GOPHER_CHAR_CSO:
        return GOPHER_TYPE_CSO;

    case GOPHER_CHAR_ERROR:
        return GOPHER_TYPE_ERROR;

    case GOPHER_CHAR_MAC_BINHEX:
        return GOPHER_TYPE_MAC_BINHEX;

    case GOPHER_CHAR_DOS_ARCHIVE:
        return GOPHER_TYPE_DOS_ARCHIVE;

    case GOPHER_CHAR_UNIX_UUENCODED:
        return GOPHER_TYPE_UNIX_UUENCODED;

    case GOPHER_CHAR_INDEX_SERVER:
        return GOPHER_TYPE_INDEX_SERVER;

    case GOPHER_CHAR_TELNET:
        return GOPHER_TYPE_TELNET;

    case GOPHER_CHAR_BINARY:
        return GOPHER_TYPE_BINARY;

    case GOPHER_CHAR_REDUNDANT:
        return GOPHER_TYPE_REDUNDANT;

    case GOPHER_CHAR_TN3270:
        return GOPHER_TYPE_TN3270;

    case GOPHER_CHAR_GIF:
        return GOPHER_TYPE_GIF;

    case GOPHER_CHAR_IMAGE:
        return GOPHER_TYPE_IMAGE;

    case GOPHER_CHAR_BITMAP:
        return GOPHER_TYPE_BITMAP;

    case GOPHER_CHAR_MOVIE:
        return GOPHER_TYPE_MOVIE;

    case GOPHER_CHAR_SOUND:     // '<'
    case GOPHER_CHAR_SOUND_2:   // 's'
        return GOPHER_TYPE_SOUND;

    case GOPHER_CHAR_HTML:
        return GOPHER_TYPE_HTML;

    case GOPHER_CHAR_PDF:
        return GOPHER_TYPE_PDF;

    case GOPHER_CHAR_CALENDAR:
        return GOPHER_TYPE_CALENDAR;

    case GOPHER_CHAR_INLINE:
        return GOPHER_TYPE_INLINE;
    }
    return GOPHER_TYPE_UNKNOWN;
}


CHAR
GopherTypeToChar(
    IN DWORD GopherType
    )

/*++

Routine Description:

    Opposite of GopherCharToType

Arguments:

    GopherType  - bitmap of attributes. Only one file type and gopher plus
                  attributes can be set simultaneously

Return Value:

    CHAR
        Success - mapped gopher char
        Failure - INVALID_GOPHER_TYPE

--*/

{
    switch (GopherType & GOPHER_TYPE_MASK) {
    case GOPHER_TYPE_TEXT_FILE:
        return GOPHER_CHAR_TEXT_FILE;

    case GOPHER_TYPE_DIRECTORY:
        return GOPHER_CHAR_DIRECTORY;

    case GOPHER_TYPE_CSO:
        return GOPHER_CHAR_CSO;

    case GOPHER_TYPE_ERROR:
        return GOPHER_CHAR_ERROR;

    case GOPHER_TYPE_MAC_BINHEX:
        return GOPHER_CHAR_MAC_BINHEX;

    case GOPHER_TYPE_DOS_ARCHIVE:
        return GOPHER_CHAR_DOS_ARCHIVE;

    case GOPHER_TYPE_UNIX_UUENCODED:
        return GOPHER_CHAR_UNIX_UUENCODED;

    case GOPHER_TYPE_INDEX_SERVER:
        return GOPHER_CHAR_INDEX_SERVER;

    case GOPHER_TYPE_TELNET:
        return GOPHER_CHAR_TELNET;

    case GOPHER_TYPE_BINARY:
        return GOPHER_CHAR_BINARY;

    case GOPHER_TYPE_REDUNDANT:
        return GOPHER_CHAR_REDUNDANT;

    case GOPHER_TYPE_TN3270:
        return GOPHER_CHAR_TN3270;

    case GOPHER_TYPE_GIF:
        return GOPHER_CHAR_GIF;

    case GOPHER_TYPE_IMAGE:
        return GOPHER_CHAR_IMAGE;

    case GOPHER_TYPE_BITMAP:
        return GOPHER_CHAR_BITMAP;

    case GOPHER_TYPE_MOVIE:
        return GOPHER_CHAR_MOVIE;

    case GOPHER_TYPE_SOUND:
        return GOPHER_CHAR_SOUND;

    case GOPHER_TYPE_HTML:
        return GOPHER_CHAR_HTML;

    case GOPHER_TYPE_PDF:
        return GOPHER_CHAR_PDF;

    case GOPHER_TYPE_CALENDAR:
        return GOPHER_CHAR_CALENDAR;

    case GOPHER_TYPE_INLINE:
        return GOPHER_CHAR_INLINE;
    }
    return UNKNOWN_GOPHER_TYPE;
}


DWORD
GetDirEntry(
    IN LPVIEW_INFO ViewInfo,
    OUT LPGOPHER_FIND_DATA FindData
    )

/*++

Routine Description:

    Retrieves the next directory entry from the current VIEW_INFO data buffer.
    The buffer pointer will be updated to point to the start of the next line
    or 1 character past the end of the buffer

Arguments:

    ViewInfo    - pointer to VIEW_INFO which points to BUFFER_INFO which
                  points to buffer containing directory listing

    FindData    - pointer to user's GOPHER_FIND_DATA structure to fill in

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NO_MORE_FILES
                    End of the directory

                  ERROR_GOPHER_PROTOCOL_ERROR

--*/

{
    DWORD error;
    char lineBuf[DEFAULT_LINE_BUFFER_LENGTH];
    DWORD lineLen;
    LPSTR linePtr;
    BOOL haveLocator;

    //
    // get the next line from the buffer. If we don't have all the data that
    // constitutes a line, ExtractLine will endeavour to get it
    //

    haveLocator = FALSE;
    lineLen = sizeof(lineBuf);
    linePtr = lineBuf;
    error = ExtractLine(ViewInfo,
                        (LPBYTE)linePtr,
                        &lineLen,
                        &ViewInfo->ViewOffset
                        );

    //
    // convert lineLen to the number of characters actually extracted, minus
    // one for the '\0'. Doesn't matter if ExtractLine() failed
    //

    lineLen = sizeof(lineBuf) - (lineLen + 1);

    //
    // if we got a line of data, but the buffer contains gopher+ info then we
    // need to move the locator pointer past the "+INFO: " token
    //

    if ((error == ERROR_SUCCESS) && (ViewInfo->Flags & VI_GOPHER_PLUS)) {

        DWORD tokenLength;

        tokenLength = IsGopherPlusToken(GOPHER_PLUS_INFO_TOKEN,
                                        GOPHER_PLUS_INFO_TOKEN_LENGTH,
                                        linePtr,
                                        lineLen
                                        );
        if (tokenLength != 0) {
            linePtr += tokenLength;
            lineLen -= tokenLength;
        } else {

            //
            // hola! The "+INFO: " doesn't exist. We'll treat this as gopher0
            // info (or return an error?)
            //

            INET_ASSERT(FALSE);

            ViewInfo->Flags &= ~VI_GOPHER_PLUS;
        }
    }

    //
    // if no error occurred, convert the locator just retrieved into the
    // GOPHER_FIND_DATA structure
    //

    if (error == ERROR_SUCCESS) {
        error = GopherLocatorToFindData(linePtr, lineLen, FindData);

        //
        // if we parsed the locator OK and the buffer contains gopher+ info
        // then we must get the date and size information from the +ADMIN
        // section Mod-Date line and +VIEWS section resp.
        //

        if ((error == ERROR_SUCCESS) && (ViewInfo->Flags & VI_GOPHER_PLUS)) {

            DWORD dataOffset;
            DWORD previousOffset;
            DWORD searchType;
            BOOL done;

            haveLocator = TRUE;

            dataOffset = ViewInfo->ViewOffset;
            searchType = 0;
            done = FALSE;

            //
            // loop, reading the next line from the directory buffer. For each
            // line, parse the gopher+ token looking for the Mod-Date line,
            // or the first view line. We just skip all other lines
            //

            do {
                previousOffset = dataOffset;
                lineLen = sizeof(lineBuf);
                error = ExtractLine(ViewInfo,
                                    (LPBYTE)linePtr,
                                    &lineLen,
                                    &dataOffset
                                    );
                if (error == ERROR_SUCCESS) {

                    //
                    // again, convert lineLen to the number of characters
                    // extracted
                    //

                    lineLen = sizeof(lineBuf) - (lineLen + 1);

                    //
                    // if we found a line containing a categpry type on the
                    // previous iteration, then parse the per-category info
                    //

                    if (searchType == SEARCH_TYPE_VIEW) {

                        char contentType[DEFAULT_CONTENT_TYPE_NAME_LENGTH + 1];
                        char language[DEFAULT_LANGUAGE_NAME_LENGTH + 1];
                        DWORD contentTypeLength;
                        DWORD languageLength;
                        BOOL ok;

                        //
                        // must be views line. Just extract the first one
                        //

                        contentTypeLength = sizeof(contentType) - 1;
                        languageLength = sizeof(language) - 1;
                        ok = ExtractView(&linePtr,
                                         contentType,
                                         &contentTypeLength,
                                         language,
                                         &languageLength,
                                         &FindData->SizeLow
                                         );

                        INET_ASSERT(ok);

                        //
                        // we have the first view line. We aren't interested in
                        // the rest
                        //

                        searchType = 0;
                    } else {

                        LPSTR pAttribute;
                        char attributeBuffer[DEFAULT_CONTENT_TYPE_NAME_LENGTH + 1];
                        int i;
                        DWORD len;
                        LPSTR argPtr;

                        //
                        // pull out the first token on the line
                        //

                        i = 0;
                        len = lineLen;
                        pAttribute = linePtr;

                        //
                        // if this line has leading space, then skip it
                        //

                        while (*pAttribute == ' ') {
                            ++pAttribute;
                            --len;
                        }
                        while (len
                        && (i < sizeof(attributeBuffer) - 1)
                        && (pAttribute[i] != ' ')
                        && (pAttribute[i] != ':')
                        && (pAttribute[i] != '\r')
                        && (pAttribute[i] != '\n')) {
                            attributeBuffer[i] = pAttribute[i];
                            ++i;
                            --len;
                        }
                        attributeBuffer[i] = '\0';
                        switch (MapAttributeNameToId((LPCSTR)attributeBuffer)) {
                        case GOPHER_CATEGORY_ID_INFO:

                            //
                            // update the offset in the VIEW_INFO
                            //

                            ViewInfo->ViewOffset = previousOffset;

                            //
                            // we have got to the next directory entry. Quit
                            //

                            done = TRUE;
                            break;

                        case GOPHER_ATTRIBUTE_ID_MOD_DATE:

                            //
                            // this is the "Mod-Date" line. Find the start of
                            // the date-time field (in angle brackets) and
                            // extract the time and date to the GOPHER_FIND_DATE
                            // structure
                            //

                            argPtr = strchr(linePtr, '<');
                            if (argPtr != NULL) {
                                ExtractDateAndTime(&argPtr,
                                                   &FindData->LastModificationTime
                                                   );
                            }
                            break;

                        case GOPHER_CATEGORY_ID_VIEWS:

                            //
                            // we have found the +VIEWS section. Next thing to
                            // find is the views proper
                            //

                            searchType = SEARCH_TYPE_VIEW;
                            break;

                        default:

                            //
                            // we just skip all other lines except the line(s)
                            // containing view information, in which case
                            // searchType will be set to indicate that this line
                            // contains a view
                            //

                            break;
                        }
                    }

                } else {

                    //
                    // ExtractLine had an error
                    //

                     ViewInfo->ViewOffset = dataOffset;
                    done = TRUE;
                }
            } while ( !done );
        }
    }
    if (error == ERROR_GOPHER_END_OF_DATA) {
        if (haveLocator) {
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_NO_MORE_FILES;
        }
    }
    return error;
}


PRIVATE
DWORD
GopherLocatorToFindData(
    IN LPCSTR Locator,
    IN DWORD Length,
    OUT LPGOPHER_FIND_DATA FindData
    )

/*++

Routine Description:

    Fills in the GOPHER_FIND_DATA fields from a gopher locator string. The
    strings in the GOPHER_FIND_DATA are appended after the fixed structure.

    ASSUMES 1. The buffer pointed to by FindData is large enough to hold the
               fixed and variable parts of the GOPHER_FIND_DATA

Arguments:

    Locator     - pointer to (ASCII) locator string

    Length      - length of Locator

    FindData    - pointer to GOPHER_FIND_DATA structure

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_GOPHER_PROTOCOL_ERROR

--*/

{
    LPSTR stringPointer;
    DWORD bufferLength;
    LPSTR locator;
    DWORD locatorLength;

    FindData->GopherType = GopherCharToType(*Locator);

    if (IsGopherPlus(Locator)) {
        FindData->GopherType |= GOPHER_TYPE_GOPHER_PLUS;
    }

    stringPointer = FindData->DisplayString;

    //
    // copy the display string into the GOPHER_FIND_DATA. We no longer care
    // about the length copied
    //

    ExtractDisplayString(Locator,
                         &stringPointer,
                         sizeof(FindData->DisplayString)
                         );

    //
    // default the size and time/date fields to zero. If we received a gopher+
    // directory list, we will fill in these fields from the attribute info
    //

    FindData->SizeLow = 0;
    FindData->SizeHigh = 0;
    FindData->LastModificationTime.dwLowDateTime = 0;
    FindData->LastModificationTime.dwHighDateTime = 0;

    //
    // copy the locator into the GOPHER_FIND_DATA
    //

    stringPointer = FindData->Locator;
    bufferLength = sizeof(FindData->Locator);
    locator = (LPSTR)Locator;
    locatorLength = Length;
    if (CopyToEol(&stringPointer, &bufferLength, &locator, &locatorLength)) {

        if (FindData->GopherType == GOPHER_TYPE_UNKNOWN) {

            DEBUG_PRINT(PARSE,
                        ERROR,
                        ("GopherLocatorToFindData(): unknown locator type: \"%s\"\n",
                        FindData->Locator
                        ));

        }

        return ERROR_SUCCESS;
    }

    //
    // CopyToEol failed to find the end-of-line in the Locator string.
    // Either something's bust, or we have received a locator that breaks
    // our locator length limit
    //

    if ((bufferLength == 0) && (locatorLength != 0)) {

        char bigLocator[2 * MAX_GOPHER_LOCATOR_LENGTH + 1];

        //
        // blown our locator length limit. We will reconstruct a slightly
        // modified (smaller) locator
        //

        stringPointer = bigLocator;
        bufferLength = sizeof(bigLocator);
        locator = (LPSTR)Locator;
        locatorLength = Length;
        if (!CopyToEol(&stringPointer, &bufferLength, &locator, &locatorLength)) {

            //
            // CopyToEol() still fails! Either this is an extremely long
            // locator, or we are not parsing a directory output
            //

            // skip to \r\n?

            INET_ASSERT(FALSE);

            return ERROR_GOPHER_DATA_ERROR;
        }

        //
        // if we think this is a locator, albeit one that breaks our internal
        // locator length limit, crack it open, and then reconstitute
        //

        if (IsValidLocator(bigLocator, sizeof(bigLocator))) {

            DWORD gopherType;
            char displayString[MAX_GOPHER_DISPLAY_TEXT * 2 + 1];
            DWORD displayStringLength;
            char selectorString[MAX_GOPHER_SELECTOR_TEXT * 2 + 1];
            DWORD selectorStringLength;
            char hostName[MAX_GOPHER_HOST_NAME * 2 + 1];
            DWORD hostNameLength;
            DWORD port;

            displayStringLength = sizeof(displayString);
            selectorStringLength = sizeof(selectorString);
            hostNameLength = sizeof(hostName);
            if (CrackLocator(bigLocator,
                             &gopherType,
                             displayString,
                             &displayStringLength,
                             selectorString,
                             &selectorStringLength,
                             hostName,
                             &hostNameLength,
                             &port,
                             NULL)) {

                //
                // we really want to ensure that only the display string is
                // broken, but we can get some weird FTP-based locators that
                // contain long selector strings. As a compromise, just add
                // an extra terminator at the relevamt maximum offset in each
                // string
                //

                displayString[MAX_GOPHER_DISPLAY_TEXT] = '\0';
                selectorString[MAX_GOPHER_SELECTOR_TEXT] = '\0';
                hostName[MAX_GOPHER_HOST_NAME] = '\0';

                //
                // and reconstruct the locator
                //

                bufferLength = sizeof(FindData->Locator);
                if (gopherType == GOPHER_TYPE_UNKNOWN) {

                    //
                    // BUGBUG - should change GopherCreateLocator() so that it
                    //          is more forgiving of 'unknown' types (accept
                    //          a character, not a bit)
                    //

                    gopherType = GOPHER_TYPE_ERROR;
                }
                if (GopherCreateLocator((LPCSTR)hostName,
                                        (INTERNET_PORT)port,
                                        (LPCSTR)displayString,
                                        (LPCSTR)selectorString,
                                        gopherType,
                                        FindData->Locator,
                                        &bufferLength)) {
                    return ERROR_SUCCESS;
                } else {

                    //
                    // GopherCreateLocator() failed
                    //

                    INET_ASSERT(FALSE);

                }
            } else {

                //
                // CrackLocator() failed
                //

                INET_ASSERT(FALSE);

            }
        } else {

            //
            // IsValidLocator() returned FALSE
            //

            INET_ASSERT(FALSE);

        }
    } else {

        //
        // ran off the end of the directory list without finding "\r\n"?
        //

        INET_ASSERT(FALSE);

    }
    return ERROR_GOPHER_DATA_ERROR;
}


DWORD
ReadData(
    IN LPVIEW_INFO ViewInfo,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Reads data from a file buffer into the caller's buffer

Arguments:

    ViewInfo        - pointer to VIEW_INFO structure

    BytesReturned   - amount of data copied to caller's buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    BytesReturned contains amount of data copied to user buffer

        Failure - ERROR_GOPHER_DATA_ERROR
                    There is an inconsistency between the VIEW_INFO and the
                    BUFFER_INFO

                  ERROR_GOPHER_END_OF_DATA
                    All data has been copied to the user buffer

                  ERROR_GOPHER_TIMEOUT
                    We got a timeout trying to communicate with the gopher
                    server

                  Win32 error
                    Returned if we have a memory or heap problem

                  WSA error
                    Socket specific error returned by ReceiveResponse()

--*/

{
    INET_ASSERT(ViewInfo != NULL);
    INET_ASSERT(ViewInfo->ViewType == ViewTypeFile);
    INET_ASSERT(ViewInfo->BufferInfo != NULL);

    if (ViewInfo->BufferInfo->Flags & BI_RECEIVE_COMPLETE) {
        *BytesReturned = 0;
        return ERROR_SUCCESS;
    } else {
        return GopherReceiveResponse(ViewInfo, BytesReturned);
    }
}


PRIVATE
DWORD
ExtractLine(
    IN LPVIEW_INFO ViewInfo,
    OUT LPBYTE LineBuffer,
    IN OUT LPDWORD LineBufferLength,
    IN OUT LPDWORD DataOffset
    )

/*++

Routine Description:

    Extracts a line from a response buffer into a local buffer. If the buffer
    does not contain all of the current line we retrieve the next chunk by
    calling ReceiveResponse()

Arguments:

    ViewInfo        - describes VIEW_INFO from which to extract line

    LineBuffer      - pointer to buffer where line will be copied

    LineBufferLength- IN: length of line buffer
                      OUT: number of bytes remaining in LineBuffer

    DataOffset      - IN: the point in the data buffer corresponding to
                      ViewInfo->BufferInfo->Buffer at which to start the
                      extraction

                      OUT: The next offset in buffer at which to start

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_GOPHER_DATA_ERROR
                    We had an error parsing the data

                  ERROR_GOPHER_END_OF_DATA
                    We reached the end of the info - API returns
                    ERROR_NO_MORE_FILES

                  Win32 error

--*/

{
    DWORD error;
    BOOL copied;
    LPBYTE startOfLine;
    LPBUFFER_INFO bufferInfo;

    INET_ASSERT(ViewInfo != NULL);
    INET_ASSERT(ViewInfo->BufferInfo != NULL);

    bufferInfo = ViewInfo->BufferInfo;
    startOfLine = LineBuffer;
    error = ERROR_SUCCESS;

    do {

        LPBYTE bufferPointer;
        LPBYTE responsePointer;
        DWORD bufferAvailable;
        DWORD oldBufferAvailable;

        if (*DataOffset > bufferInfo->BufferLength) {

            //
            // we think we're further into buffer than there is available data
            //

            INET_ASSERT(FALSE);

            error = ERROR_GOPHER_DATA_ERROR;

            DEBUG_PRINT(PARSE,
                        ERROR,
                        ("ExtractLine(): *DataOffset (%d) > BufferLength (%d)\n",
                        *DataOffset,
                        bufferInfo->BufferLength
                        ));

            goto quit;
        }

        if ((bufferInfo->Flags & BI_RECEIVE_COMPLETE)
        && (*DataOffset == bufferInfo->BufferLength)) {

            //
            // the caller has already reached the end of the buffer
            //

            DEBUG_PRINT(PARSE,
                        INFO,
                        ("ExtractLine(): already at EOF buffer\n"
                        ));

            error = ERROR_GOPHER_END_OF_DATA;
            goto quit;
        }

        //
        // get a pointer to the start of the buffer
        //

        bufferPointer = bufferInfo->Buffer;

        INET_ASSERT(bufferPointer != NULL);

        if (bufferPointer == NULL) {
            goto last_error_exit;
        }

        //
        // now point to the offset in the buffer where the caller thinks the
        // next line begins and reduce the buffer length by the same amount
        //

        responsePointer = bufferPointer + *DataOffset;
        bufferAvailable = bufferInfo->BufferLength - *DataOffset;

        //
        // copy from the current buffer position to the end of the line
        //

        oldBufferAvailable = bufferAvailable;
        copied = CopyToEol((LPSTR *)&LineBuffer,
                           LineBufferLength,
                           (LPSTR *)&responsePointer,
                           &bufferAvailable
                           );

        //
        // oldBufferAvailable - bufferAvailable is the amount we copied
        //

        *DataOffset += oldBufferAvailable - bufferAvailable;

        //
        // copied is TRUE if CopyToEol copied a full line
        //

        if (copied) {

            //
            // test again for ".\r\n" terminator. Some bozo servers terminate
            // with ".\r\r\n" which would have escaped our test in
            // ReceiveResponse(), but has now been compressed to ".\r\n" by
            // CopyToEol(). Other, equally brain-damaged servers, terminate
            // with e.g. ".\r\n\x1a"  (presumably this directory was read from
            // a file and squirted out via send())
            //

            if (memcmp(startOfLine, ".\r\n", 3) == 0) {

                //
                // there should be very few bytes left in the buffer, if any,
                // depending on how the server terminated the buffer (".\r\r\n"
                // or ".\r\n\x1a", e.g.)
                //
                // N.B. 8 is an arbitrary number. I don't expect too many
                // garbage characters at the end of the buffer, but if there's
                // more than a relatively small number, we could have a ".\r\n"
                // embedded half-way down the directory listing. Implausable,
                // yes, but then there's nothing so unpredictable as the results
                // from a gopher server
                //
                // 05/23/95
                //
                // server at sutro.sfsu.edu returns a pile of garbage after the
                // end-of-buffer mark. Probably unintentional, but causes the
                // following assertion to go off:
                //
                //  INET_ASSERT(bufferInfo->BufferLength - *DataOffset <= 8);
                //

                if (bufferInfo->BufferLength - *DataOffset <= 8) {

                    DEBUG_PRINT(PARSE,
                                WARNING,
                                ("ExtractLine(): Buffer handle %#x contains data after end-of-buffer mark\n",
                                bufferInfo->Buffer
                                ));
                }

                error = ERROR_GOPHER_END_OF_DATA;
            }
        } else {

            //
            // at the time we called CopyToEol, all of the current line was
            // not in the response buffer. Get the next part of the response
            //

            AcquireBufferLock(bufferInfo);
            if (!(bufferInfo->Flags & BI_RECEIVE_COMPLETE)) {

                DWORD bytesReceived;

                error = GopherReceiveResponse(ViewInfo, &bytesReceived);
            }
            ReleaseBufferLock(bufferInfo);
        }
    } while (!copied && (error == ERROR_SUCCESS));

quit:

    return error;

last_error_exit:

    error = GetLastError();
    goto quit;
}


PRIVATE
DWORD
ExtractDisplayString(
    IN LPCSTR Locator,
    IN OUT LPSTR* StringPointer,
    IN DWORD BufferLength
    )

/*++

Routine Description:

    Given a gopher locator string, extract the display string part

Arguments:

    Locator         - pointer to gopher locator

    StringPointer   - pointer to pointer to output string. Updated on output

    BufferLength    - amount of space in *StringPointer

Return Value:

    DWORD   Length of string extracted

--*/

{
    LPSTR originalPointer = *StringPointer;
    char ch;

    //
    // Locator starts off pointing at the type character. Move past it then
    // copy everything up to the tab character
    //

    while (((ch = *++Locator) != '\t') && BufferLength--) {
        *(*StringPointer)++ = ch;
    }
    *(*StringPointer)++ = '\0';
    return (DWORD) (*StringPointer - originalPointer);
}


BOOL
CopyToEol(
    IN OUT LPSTR* Destination,
    IN OUT LPDWORD DestinationLength,
    IN OUT LPSTR* Source,
    IN OUT LPDWORD SourceLength
    )

/*++

Routine Description:

    Copies the current gopher response line up to the end of the current line
    in the buffer. The destination string is zero terminated if TRUE is
    returned

    On exit, all parameters are updated to reflect the current positions and
    lengths of the buffers so this function can be called iteratively until
    the entire line is copied

    ASSUMES 1. The Length is absolutely reliable - i.e. when Length == 2 and
               **Source == '\r', then *(*Source + 1) == '\n'

Arguments:

    Destination         - pointer to place to copy to

    DestinationLength   - pointer to length of destination buffer, updated on output

    Source              - pointer to place to copy from (gopher response buffer)

    SourceLength        - pointer tp length of source buffer, updated on output

Return Value:

    BOOL
        TRUE    - we copied the entire line up to \r\n

        FALSE   - none or part of a line was copied

--*/

{
    LPSTR src;
    LPSTR dest;
    DWORD srcLen;
    DWORD destLen;
    BOOL copied;

    //
    // make smaller code (i.e. don't deref the parms every time)
    //

    src = *Source;
    dest = *Destination;
    srcLen = *SourceLength;
    destLen = *DestinationLength;

    while ((*src != '\r') && (*src != '\n') && (destLen != 0) && (srcLen != 0)) {
        *dest++ = *src++;
        --destLen;
        --srcLen;
    }

    //
    // we can receive multiple carriage-returns, presumably because the server
    // uses sprintf("\r\n") in text mode which expands "\n" to be \r\n in the
    // output buffer. We will collapse multiple carriage-returns
    //

    while ((*(src + 1) == '\r') && (srcLen != 0)) {
        ++src;
        --srcLen;
    }

    //INET_ASSERT((srcLen > 1) ? (*(src + 1) == '\n') : TRUE);

    //
    // if \r\n exist in the source buffer then copy them and update the length
    //

    copied = FALSE;
    if (destLen >= 3) {
        if ((srcLen >= 2) && (*src == '\r')) {
            ++src;
            --srcLen;
        }
        if ((srcLen >= 1) && (*src == '\n')) {
            ++src;
            --srcLen;

            //
            // we have reached a line-feed. It either exists on its own or was
            // prefixed by a carriage-return. This is the end of the line...
            //
            // Note, even if we did not find \r\n in the source, we create \r\n
            // in the destination
            //

            destLen -= 3;   // 1 for \r, 1 for \n, 1 for \0
            *dest++ = '\r';
            *dest++ = '\n';
            *dest++ = '\0';
            copied = TRUE;
        }
    }

    *Source = src;
    *Destination = dest;
    *SourceLength = srcLen;
    *DestinationLength = destLen;
    return copied;
}


DWORD
IsGopherPlusToken(
    IN LPSTR Token,
    IN DWORD TokenLength,
    IN LPSTR Buffer,
    IN DWORD BufferLength
    )

/*++

Routine Description:

    Determines if a token is the gopher+ token. In order to match we need to
    match a trailing space character also. Seems that some servers return
    "+INFO" and some "+INFO:". We must handle both

Arguments:

    Token           - pointer to gopher+ token string

    TokenLength     - length of PlusToken

    Buffer          - pointer to buffer containing token to check

    BufferLength    - number of bytes in BufferPointer

Return Value:

    DWORD
        Success - Returns number of characters matched, including trailing ' '

        Failure - 0. TokenPointer does not point at PlusToken or we ran out of
                  buffer before we could make the distinction

--*/

{
    //
    // Length must contain at least the trailing space, and possibly a ':'
    //

    if (BufferLength >= TokenLength + 2) {
        if (memcmp(Buffer, Token, TokenLength) == 0) {
            if (Buffer[TokenLength] == ':') {
                ++TokenLength;
            }

            //
            // if there's a space after the token then we know its really what we
            // are searching for
            //

            if (Buffer[TokenLength] == ' ') {
                return ++TokenLength;
            }
        }
    }
    return 0;
}


DWORD
MapAttributeNameToId(
    IN LPCSTR AttributeName
    )

/*++

Routine Description:

    Given a category or attribute name, returns an identifier to avoid having
    to perform extraneous string comparisons

    ASSUMES:    1. AttributeName has correct case. Comparisons are CASE-SENSITIVE

Arguments:

    AttributeName   - name of category or attribute

Return Value:

    DWORD
        relevant identifer or GOPHER_ATTRIBUTE_ID_UNKNOWN

--*/

{
    if (AttributeName == NULL) {
        return GOPHER_CATEGORY_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_INFO_CATEGORY)) {
        return GOPHER_CATEGORY_ID_INFO;
    } else if (!stricmp(AttributeName, GOPHER_ADMIN_CATEGORY)) {
        return GOPHER_CATEGORY_ID_ADMIN;
    } else if (!stricmp(AttributeName, GOPHER_VIEWS_CATEGORY)) {
        return GOPHER_CATEGORY_ID_VIEWS;
    } else if (!stricmp(AttributeName, GOPHER_ABSTRACT_CATEGORY)) {
        return GOPHER_CATEGORY_ID_ABSTRACT;
    } else if (!stricmp(AttributeName, GOPHER_VERONICA_CATEGORY)) {
        return GOPHER_CATEGORY_ID_VERONICA;
    } else if (!stricmp(AttributeName, GOPHER_ADMIN_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_ADMIN;
    } else if (!stricmp(AttributeName, GOPHER_MOD_DATE_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_MOD_DATE;
    } else if (!stricmp(AttributeName, GOPHER_TTL_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_TTL;
    } else if (!stricmp(AttributeName, GOPHER_SCORE_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_SCORE;
    } else if (!stricmp(AttributeName, GOPHER_RANGE_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_RANGE;
    } else if (!stricmp(AttributeName, GOPHER_SITE_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_SITE;
    } else if (!stricmp(AttributeName, GOPHER_ORG_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_ORG;
    } else if (!stricmp(AttributeName, GOPHER_LOCATION_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_LOCATION;
    } else if (!stricmp(AttributeName, GOPHER_GEOG_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_GEOG;
    } else if (!stricmp(AttributeName, GOPHER_TIMEZONE_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_TIMEZONE;
    } else if (!stricmp(AttributeName, GOPHER_PROVIDER_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_PROVIDER;
    } else if (!stricmp(AttributeName, GOPHER_VERSION_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_VERSION;
    } else if (!stricmp(AttributeName, GOPHER_ABSTRACT_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_ABSTRACT;
    } else if (!stricmp(AttributeName, GOPHER_VIEW_ATTRIBUTE)) {
        return GOPHER_ATTRIBUTE_ID_VIEW;
    }
    return GOPHER_ATTRIBUTE_ID_UNKNOWN;
}

#if defined(GOPHER_ATTRIBUTE_SUPPORT)


VOID
MapAttributeToIds(
    IN LPCSTR AttributeName,
    OUT LPDWORD CategoryId,
    OUT LPDWORD AttributeId
    )

/*++

Routine Description:

    Given a category or attribute name, returns an identifier to avoid having
    to perform extraneous string comparisons

    ASSUMES:    1. AttributeName has correct case. Comparisons are CASE-SENSITIVE

Arguments:

    AttributeName   - name of category or attribute

    CategoryId      - returned GOPHER_CATEGORY_ id

    AttributeId     - returned GOPHER_ATTRIBUTE_ id

Return Value:

    None.

--*/

{
    DWORD category;
    DWORD attribute;

    if (AttributeName == NULL) {
        category = GOPHER_CATEGORY_ID_ALL;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_INFO_CATEGORY)) {
        category = GOPHER_CATEGORY_ID_INFO;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_ADMIN_CATEGORY)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_VIEWS_CATEGORY)) {
        category = GOPHER_CATEGORY_ID_VIEWS;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_ABSTRACT_CATEGORY)) {
        category = GOPHER_CATEGORY_ID_ABSTRACT;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_VERONICA_CATEGORY)) {
        category = GOPHER_CATEGORY_ID_VERONICA;
        attribute = GOPHER_ATTRIBUTE_ID_ALL;
    } else if (!stricmp(AttributeName, GOPHER_ADMIN_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_ADMIN;
    } else if (!stricmp(AttributeName, GOPHER_MOD_DATE_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_MOD_DATE;
    } else if (!stricmp(AttributeName, GOPHER_TTL_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_TTL;
    } else if (!stricmp(AttributeName, GOPHER_SCORE_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_SCORE;
    } else if (!stricmp(AttributeName, GOPHER_RANGE_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_RANGE;
    } else if (!stricmp(AttributeName, GOPHER_SITE_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_SITE;
    } else if (!stricmp(AttributeName, GOPHER_ORG_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_ORG;
    } else if (!stricmp(AttributeName, GOPHER_LOCATION_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_LOCATION;
    } else if (!stricmp(AttributeName, GOPHER_GEOG_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_CATEGORY_ID_ADMIN;
    } else if (!stricmp(AttributeName, GOPHER_TIMEZONE_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_TIMEZONE;
    } else if (!stricmp(AttributeName, GOPHER_PROVIDER_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_PROVIDER;
    } else if (!stricmp(AttributeName, GOPHER_VERSION_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ADMIN;
        attribute = GOPHER_ATTRIBUTE_ID_VERSION;
    } else if (!stricmp(AttributeName, GOPHER_ABSTRACT_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_ABSTRACT;
        attribute = GOPHER_ATTRIBUTE_ID_ABSTRACT;
    } else if (!stricmp(AttributeName, GOPHER_VIEW_ATTRIBUTE)) {
        category = GOPHER_CATEGORY_ID_VIEWS;
        attribute = GOPHER_ATTRIBUTE_ID_VIEW;
    } else {
        category = GOPHER_CATEGORY_ID_UNKNOWN;
        attribute = GOPHER_ATTRIBUTE_ID_UNKNOWN;
    }
    *CategoryId = category;
    *AttributeId = attribute;
}


BOOL
MapAttributeIdToNames(
    IN DWORD AttributeId,
    OUT LPSTR* CategoryName,
    OUT LPSTR* AttributeName
    )

/*++

Routine Description:

    Do reverse transformation: given attribute ID, return the category and
    attribute names if known

Arguments:

    AttributeId     - id to map

    CategoryName    - pointer to pointer to category name

    AttributeName   - pointer to pointer to attribute name

Return Value:

    BOOL
        TRUE    - id was mapped

        FALSE   - id not recognized

--*/

{
    BOOL success = TRUE;

    switch (AttributeId) {
    case GOPHER_CATEGORY_ID_ALL:
        *CategoryName = NULL;
        *AttributeName = NULL;
        break;

    case GOPHER_CATEGORY_ID_INFO:
        *CategoryName = GOPHER_INFO_CATEGORY;
        *AttributeName = NULL;
        break;

    case GOPHER_CATEGORY_ID_ADMIN:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = NULL;
        break;

    case GOPHER_CATEGORY_ID_VIEWS:
        *CategoryName = GOPHER_VIEWS_CATEGORY;
        *AttributeName = NULL;
        break;

    case GOPHER_CATEGORY_ID_ABSTRACT:
        *CategoryName = GOPHER_ABSTRACT_CATEGORY;
        *AttributeName = NULL;
        break;

    case GOPHER_CATEGORY_ID_VERONICA:
        *CategoryName = GOPHER_VERONICA_CATEGORY;
        *AttributeName = NULL;
        break;

    case GOPHER_ATTRIBUTE_ID_ADMIN:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_ADMIN_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_MOD_DATE:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_MOD_DATE_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_TTL:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_TTL_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_SCORE:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_SCORE_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_RANGE:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_RANGE_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_SITE:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_SITE_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_ORG:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_ORG_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_LOCATION:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_LOCATION_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_GEOG:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_GEOG_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_TIMEZONE:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_TIMEZONE_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_PROVIDER:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_PROVIDER_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_VERSION:
        *CategoryName = GOPHER_ADMIN_CATEGORY;
        *AttributeName = GOPHER_VERSION_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_ABSTRACT:
        *CategoryName = GOPHER_ABSTRACT_CATEGORY;
        *AttributeName = GOPHER_ABSTRACT_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_VIEW:
        *CategoryName = GOPHER_VIEWS_CATEGORY;
        *AttributeName = GOPHER_VIEW_ATTRIBUTE;
        break;

    case GOPHER_ATTRIBUTE_ID_UNKNOWN:
        success = FALSE;
        break;

    default:
        success = FALSE;
        break;
    }
    return success;
}

#endif // defined(GOPHER_ATTRIBUTE_SUPPORT)


DWORD
GetGopherNumber(
    IN OUT LPSTR* pString
    )

/*++

Routine Description:

    Converts a 'gopher number' to a DWORD. A gopher number is the value usually
    contained with angle brackets in e.g. a +VIEWS line, and is usually a
    fractional number with a 'k' suffix

Arguments:

    pString - pointer to pointer to string which points at the start of the
              number. The number may start with a period, indicating that it
              is less than one (hence the reason why we use a double).

              On output, the parameter points at the character after what we
              took to be the number

Return Value:

    DWORD
        DWORD representation of the number at *pString

--*/

{

    double number;

    //
    // the gopher number is usually inside angle brackets. Move the string
    // pointer past the opening bracket, if the caller has not already
    // done so
    //

    if (**pString == '<') {
        ++*pString;
    }

    //
    // allow strtod to move the string pointer forward
    //

    number = StrToDbl(*pString, pString);

    return (DWORD)number;
}


BOOL
ExtractDateAndTime(
    IN OUT LPSTR* pString,
    OUT LPFILETIME pFileTime
    )

/*++

Routine Description:

    Converts a 'gopher time-and-date' field to a WIN32 FILETIME structure. The
    gopher date-time field is a string representation of the date and time,
    contained within angle brackets and has the following format:

        <YYYYMMDDhhmmss>

    Where:

        YYYY = year (e.g. "1995")
        MM   = month (1..12)
        DD   = day of month
        hh   = hour of day in 24-hour format
        mm   = minute of hour
        ss   = second of minute

    Assumes:    1. On input, *pString contains entire field

Arguments:

    pString     - IN: points to the first character in the date-time field
                  OUT: points to the next character in the input stream

    pFileTime   - pointer to returned FILETIME structure

Return Value:

    BOOL
        Success - TRUE  - field was converted

        Failure - FALSE - a problem occurred while parsing the field

--*/

{
    SYSTEMTIME systemTime;

    if (**pString == '<') {
        ++*pString;
    }
    if (ExtractWord(pString, 4, &systemTime.wYear)
    && ExtractWord(pString, 2, &systemTime.wMonth)
    && ExtractWord(pString, 2, &systemTime.wDay)
    && ExtractWord(pString, 2, &systemTime.wHour)
    && ExtractWord(pString, 2, &systemTime.wMinute)
    && ExtractWord(pString, 2, &systemTime.wSecond)) {

        INET_ASSERT(**pString == '>');

        ++*pString;
        systemTime.wDayOfWeek = 0;
        systemTime.wMilliseconds = 0;
        return SystemTimeToFileTime(&systemTime, pFileTime);
    } else {

        INET_ASSERT(FALSE);

        return FALSE;
    }
}


BOOL
ExtractView(
    IN OUT LPSTR* pString,
    OUT LPSTR ContentType,
    IN OUT LPDWORD ContentTypeLength,
    OUT LPSTR Language,
    IN OUT LPDWORD LanguageLength,
    OUT LPDWORD Size
    )

/*++

Routine Description:

    Given a pointer to a line containing a view, parse it into its constituent
    parts. A view line has the following format:

        "{space}{content-type}{space}[{language-id}:]<{size}><CR><LF>"

    The language-id field is optional

Arguments:

    pString             - pointer to pointer to view line

    ContentType         - pointer to returned MIME content-type string

    ContentTypeLength   - IN: size of content-type buffer
                          OUT: length of content-type without terminating 0

    Language            - pointer to ISO-693 language-id (or NUL string)

    LanguageLength      - IN: size of language buffer
                          OUT: length of language without terminating 0

    Size                - pointer to returned size of view

Return Value:

    BOOL

--*/

{
    LPSTR string;

    string = *pString;
    while (*string == ' ') {
        ++string;
    }
    while (*string != ' ') {
        *ContentType++ = *string++;
    }
    *ContentType = '\0';
    while (*string == ' ') {
        ++string;
    }
    if (*string != '<') {

        //
        // must be the language field. Copy up to the terminating ':' or ' '
        //

        while ((*string != ' ') && (*string != ':')) {
            *Language++ = *string++;
        }

        //
        // move the string pointer to the start of the size field
        //

        while (*string != '<') {
            ++string;
        }
    }
    *Language = '\0';
    *Size = GetGopherNumber(&string);
    *pString = string;
    return TRUE;
}

#if defined(GOPHER_ATTRIBUTE_SUPPORT)

//
// manifests
//

#define SIZE_OF_GOPHER_ATTRIBUTE_FIXED_PART (2 * sizeof(DWORD))

//
// private types
//

typedef struct {
    DWORD CategoryId;
    DWORD AttributeId;
    DWORD (*Parser)(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
    DWORD NumberOfFields;
    DWORD FixedSize;
} ATTRIBUTE_PARSER, *LPATTRIBUTE_PARSER;

typedef struct {
    LPCSTR String;
} SINGLE_STRING_TYPE, *LPSINGLE_STRING_TYPE;

//
// private parser prototypes
//

PRIVATE DWORD ParseAdminAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseModDateAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseAbstractAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseViewAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseTreewalkAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseIntField(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseDwordField(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseStringField(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);
PRIVATE DWORD ParseUnknownAttribute(LPSTR*, LPDWORD, LPBYTE, LPDWORD, DWORD, DWORD);

//
// data
//

ATTRIBUTE_PARSER AttributeParsers[] = {
    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_ADMIN,
            ParseAdminAttribute,
                2,
                    sizeof(GOPHER_ADMIN_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_MOD_DATE,
            ParseModDateAttribute,
                1,
                    sizeof(GOPHER_MOD_DATE_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_TTL,
            ParseDwordField,
                1,
                    sizeof(GOPHER_TTL_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_SCORE,
            ParseIntField,
                1,
                    sizeof(GOPHER_SCORE_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_RANGE,
            ParseIntField,
                2,
                    sizeof(GOPHER_SCORE_RANGE_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_SITE,
            ParseStringField,
                1,
                    sizeof(GOPHER_SITE_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_ORG,
            ParseStringField,
                1,
                    sizeof(GOPHER_ORGANIZATION_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_LOCATION,
            ParseStringField,
                1,
                    sizeof(GOPHER_LOCATION_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_GEOG,
            ParseIntField,
                6,
                    sizeof(GOPHER_GEOGRAPHICAL_LOCATION_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_TIMEZONE,
            ParseIntField,
                1,
                    sizeof(GOPHER_TIMEZONE_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_PROVIDER,
            ParseStringField,
                1,
                    sizeof(GOPHER_PROVIDER_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ADMIN,
        GOPHER_ATTRIBUTE_ID_VERSION,
            ParseStringField,
                1,
                    sizeof(GOPHER_VERSION_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_ABSTRACT,
        GOPHER_ATTRIBUTE_ID_ABSTRACT,
            ParseAbstractAttribute,
                2,
                    sizeof(GOPHER_ABSTRACT_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_VIEWS,
        GOPHER_ATTRIBUTE_ID_VIEW,
            ParseViewAttribute,
                3,
                    sizeof(GOPHER_VIEW_ATTRIBUTE_TYPE),

    GOPHER_CATEGORY_ID_VERONICA,
        GOPHER_ATTRIBUTE_ID_TREEWALK,
            ParseTreewalkAttribute,
                1,
                    sizeof(GOPHER_VERONICA_ATTRIBUTE_TYPE),

    //
    // N.B. Unknown must be the last parser in the list
    //

    GOPHER_CATEGORY_ID_UNKNOWN,
        GOPHER_ATTRIBUTE_ID_UNKNOWN,
            ParseUnknownAttribute,
                1,
                sizeof(GOPHER_UNKNOWN_ATTRIBUTE_TYPE)
};

#define NUMBER_OF_PARSERS   ARRAY_ELEMENTS(AttributeParsers)

//
// functions
//


BOOL
FindAttribute(
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN LPCSTR AttributeName,
    IN OUT LPSTR* Buffer,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    CategoryId      -
    AttributeId     -
    AttributeName   -
    Buffer          -
    BufferLength    -

Return Value:

    BOOL

--*/

{
    BOOL ok;
    LPSTR categoryName;
    LPSTR attributeName;
    char searchName[DEFAULT_ATTRIBUTE_NAME_LENGTH + 1];
    DWORD id;
    int index;
    int len;

    if (AttributeId == GOPHER_ATTRIBUTE_ID_ALL) {
        id = CategoryId;
    } else {
        id = AttributeId;
    }
    ok = MapAttributeIdToNames(id, &categoryName, &attributeName);
    if (!ok) {
        attributeName = (LPSTR)AttributeName;
    }
    if (AttributeId != GOPHER_ATTRIBUTE_ID_ALL) {
        searchName[0] = ' ';
        index = 1;
    } else {
        index = 0;
        attributeName = categoryName;
    }
    len = strlen(attributeName);
    if (len >= sizeof(searchName)) {
        return FALSE;
    }
    strcpy(&searchName[index], attributeName);
    len += index;
    if (AttributeId != GOPHER_ATTRIBUTE_ID_ALL) {
        searchName[len++] = ':';
        searchName[len] = '\0';
    }
    do {
        if (*BufferLength < (DWORD)len) {
            return FALSE;
        }
        if (memcmp(*Buffer, searchName, len) == 0) {
            return TRUE;
        }
        SkipLine(Buffer, BufferLength);
    } while (*BufferLength != 0);
    return FALSE;
}


VOID
FindNextAttribute(
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN OUT LPSTR* Buffer,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    CategoryId      -
    AttributeId     -
    Buffer          -
    BufferLength    -

Return Value:

    None.

--*/

{
    BOOL found;

    INET_ASSERT((**Buffer == '+') || (**Buffer == ' '));

    if (CategoryId == GOPHER_CATEGORY_ID_UNKNOWN) {
        AttributeId = !GOPHER_ATTRIBUTE_ID_ALL;
    }

    //
    // loop looking at the next line until we find:
    //
    //  a) the end of the buffer
    //  b) the next section line (starts with '+')
    //  c) the next attribute line (starts with ' ')
    //

    for (found = FALSE; !found; ) {
        if (SkipLine(Buffer, BufferLength)) {
            if (AttributeId == GOPHER_ATTRIBUTE_ID_ALL) {
                found = (BOOL)(**Buffer == '+');
            } else {
                found = TRUE;
            }
        } else {

            //
            // end of buffer
            //

            found = TRUE;
        }
    }
}


DWORD
EnumerateAttribute(
    IN GOPHER_ATTRIBUTE_ENUMERATOR Enumerator,
    IN LPSTR LinePtr,
    IN DWORD LineLength,
    IN LPBYTE Buffer,
    IN DWORD BufferLength,
    OUT LPBOOL ResumeEnumeration
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Enumerator          -
    LinePtr             -
    LineLength          -
    Buffer              -
    BufferLength        -
    ResumeEnumeration   -

Return Value:

    DWORD

--*/

{
    DWORD error;
    char attributeName[DEFAULT_ATTRIBUTE_NAME_LENGTH + 1];
    DWORD nameLength;

    nameLength = sizeof(attributeName);
    error = ExtractAttributeName(attributeName,
                                 &nameLength,
                                 &LinePtr,
                                 &LineLength
                                 );
    if (error == ERROR_SUCCESS) {

        int i;
        DWORD categoryId;
        DWORD attributeId;
        BOOL found;

        MapAttributeToIds((LPCSTR)attributeName,
                          &categoryId,
                          &attributeId
                          );

        //
        // loop, looking fot the parser to handle this particular type. If we
        // don't find it, we will be conveniently left at the unknown parser
        // (that's why we have (NUMBER_OF_PARSERS - 1) and the unknown parser
        // at the end of the list)
        //

        for (i = 0; i < NUMBER_OF_PARSERS - 1; ++i) {
            if ((AttributeParsers[i].CategoryId == categoryId)
            && (AttributeParsers[i].AttributeId == attributeId)) {
                break;
            }
        }
        if (BufferLength >= SIZE_OF_GOPHER_ATTRIBUTE_FIXED_PART) {
            ((LPGOPHER_ATTRIBUTE_TYPE)Buffer)->CategoryId = categoryId;
            ((LPGOPHER_ATTRIBUTE_TYPE)Buffer)->AttributeId = attributeId;

            //
            // remove the fixed part from the buffer size before converting
            // the attribute
            //

            BufferLength -= SIZE_OF_GOPHER_ATTRIBUTE_FIXED_PART;
        } else {
            BufferLength = 0;
        }
        error = AttributeParsers[i].Parser(
                    &LinePtr,
                    &LineLength,
                    (LPBYTE)&((LPGOPHER_ATTRIBUTE_TYPE)Buffer)->AttributeType,
                    &BufferLength,
                    AttributeParsers[i].NumberOfFields,
                    AttributeParsers[i].FixedSize
                    );

        //
        // add back the amount of buffer space used by/required for the
        // fixed part of the GOPHER_ATTRIBUTE_TYPE structure
        //

        BufferLength + SIZE_OF_GOPHER_ATTRIBUTE_FIXED_PART;
        *ResumeEnumeration = Enumerator((LPGOPHER_ATTRIBUTE_TYPE)Buffer, error);
    }
    return error;
}


PRIVATE
DWORD
ParseIntField(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD error;

    if (*BufferLength < FixedSize) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        error = ERROR_SUCCESS;
    }
    *BufferLength = FixedSize;
    while ((error == ERROR_SUCCESS) && NumberOfFields--) {
        if (SkipLeading(LinePtr, LineLength)) {
            ExtractInt(LinePtr, 0, (LPINT)Buffer);
            Buffer = (LPBYTE)((LPINT)Buffer + 1);
        } else {
            error = ERROR_GOPHER_DATA_ERROR;
        }
    }
    return error;
}


PRIVATE
DWORD
ParseDwordField(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD error;

    if (*BufferLength < FixedSize) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        error = ERROR_SUCCESS;
    }
    *BufferLength = FixedSize;
    while ((error == ERROR_SUCCESS) && NumberOfFields--) {
        if (SkipLeading(LinePtr, LineLength)) {
            ExtractDword(LinePtr, 0, (LPDWORD)Buffer);
            Buffer = (LPBYTE)((LPDWORD)Buffer + 1);
        } else {
            error = ERROR_GOPHER_DATA_ERROR;
        }
    }
    return error;
}


PRIVATE
DWORD
ParseStringField(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD structureSize;
    DWORD error;
    DWORD stringLength;
    LPSTR stringPtr;
    LPSTR* fieldPtr;

    structureSize = 0;
    error = ERROR_SUCCESS;
    stringPtr = (LPSTR)((LPSTR)Buffer + NumberOfFields);
    fieldPtr = (LPSTR*)Buffer;
    while (NumberOfFields--) {
        SkipLeading(LinePtr, LineLength);
        stringLength = CharacterCount(LinePtr, LineLength, "\r\n");
        structureSize = sizeof(LPSTR)
                      + stringLength
                      + 1
                      ;
        if (*BufferLength >= structureSize) {
            *fieldPtr++ = stringPtr;
            CopyString(&stringPtr, *LinePtr, stringLength);
            *LinePtr += stringLength - 1;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    *BufferLength = structureSize;
    return error;
}


PRIVATE
DWORD
ParseAdminAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    LPSTR comment;
    DWORD commentLength;
    LPSTR emailAddress;
    DWORD emailAddressLength;
    LPSTR pstr;
    DWORD len;
    DWORD structureSize;
    DWORD error;

    SkipLeading(LinePtr, LineLength);
    comment = *LinePtr;
    emailAddress = strchr(*LinePtr, '<');
    if (emailAddress == NULL) {
        return ERROR_GOPHER_DATA_ERROR;
    }
    ++emailAddress;
    pstr = emailAddress;
    emailAddressLength = 0;
    len = *LineLength;
    while ((*pstr != '>') && (len != 0)) {
        ++pstr;
        ++emailAddressLength;
    }
    commentLength = (DWORD)(emailAddress - comment);
    structureSize = sizeof(GOPHER_ADMIN_ATTRIBUTE_TYPE)
                  + commentLength + 1
                  + emailAddressLength + 1
                  ;
    if (*BufferLength < structureSize) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        error = ERROR_SUCCESS;
    }
    *BufferLength = structureSize;
    if (error == ERROR_SUCCESS) {

        LPGOPHER_ADMIN_ATTRIBUTE_TYPE pStruct;
        LPSTR stringPtr;

        pStruct = (LPGOPHER_ADMIN_ATTRIBUTE_TYPE)Buffer;
        stringPtr = (LPSTR)pStruct + sizeof(GOPHER_ADMIN_ATTRIBUTE_TYPE);
        pStruct->Comment = (LPCSTR)stringPtr;
        CopyString(&stringPtr, comment, commentLength);
        pStruct->EmailAddress = (LPCSTR)stringPtr;
        CopyString(&stringPtr, emailAddress, emailAddressLength);
    }
    return error;
}


PRIVATE
DWORD
ParseModDateAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD structureSize;
    DWORD error;

    structureSize = sizeof(GOPHER_MOD_DATE_ATTRIBUTE_TYPE);
    if (*BufferLength < structureSize) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        error = ERROR_SUCCESS;
    }
    *BufferLength = structureSize;
    if (error == ERROR_SUCCESS) {

        LPSTR dateField;

        dateField = strchr(*LinePtr, '<');
        if (dateField != NULL) {
            ExtractDateAndTime(
                &dateField,
                &((LPGOPHER_MOD_DATE_ATTRIBUTE_TYPE)Buffer)->DateAndTime
                );
        } else {
            error = ERROR_GOPHER_DATA_ERROR;
        }
    }
    return error;
}


PRIVATE
DWORD
ParseAbstractAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD error;
    error = ERROR_SUCCESS;
    return error;
}


PRIVATE
DWORD
ParseViewAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    char contentType[DEFAULT_CONTENT_TYPE_NAME_LENGTH + 1];
    DWORD contentTypeLength;
    char language[DEFAULT_LANGUAGE_NAME_LENGTH + 1];
    DWORD languageLength;
    DWORD viewSize;
    BOOL ok;
    DWORD error;

    contentTypeLength = sizeof(contentType);
    languageLength = sizeof(language);
    SkipLeading(LinePtr, LineLength);
    ok = ExtractView(LinePtr,
                     contentType,
                     &contentTypeLength,
                     language,
                     &languageLength,
                     &viewSize
                     );
    if (ok) {

        DWORD structureSize;

        contentTypeLength = sizeof(contentType) - contentTypeLength;
        languageLength = sizeof(language) - languageLength;
        structureSize = sizeof(GOPHER_VIEW_ATTRIBUTE_TYPE)
                      + contentTypeLength
                      + languageLength
                      ;
        if (*BufferLength >= structureSize) {

            LPSTR stringPtr;

            stringPtr = (LPSTR)((LPGOPHER_VIEW_ATTRIBUTE_TYPE)Buffer + 1);
            ((LPGOPHER_VIEW_ATTRIBUTE_TYPE)Buffer)->ContentType = stringPtr;
            memcpy(stringPtr, contentType, contentTypeLength);
            stringPtr += contentTypeLength;
            ((LPGOPHER_VIEW_ATTRIBUTE_TYPE)Buffer)->Language = stringPtr;
            memcpy(stringPtr, language, languageLength);
            ((LPGOPHER_VIEW_ATTRIBUTE_TYPE)Buffer)->Size = viewSize;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        *BufferLength = structureSize;
    } else {
        error = ERROR_GOPHER_DATA_ERROR;
    }
    return error;
}


PRIVATE
DWORD
ParseTreewalkAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD structureSize;
    DWORD error;

    structureSize = sizeof(GOPHER_VERONICA_ATTRIBUTE_TYPE);
    if (*BufferLength < structureSize) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        error = ERROR_SUCCESS;
    }
    *BufferLength = structureSize;
    if (error == ERROR_SUCCESS) {
        SkipLeading(LinePtr, LineLength);
        if (*LineLength >= 3) {

            BOOL ok;

            ok = (BOOL)(_strnicmp(*LinePtr, "YES", 3) == 0);

            ((LPGOPHER_VERONICA_ATTRIBUTE_TYPE)Buffer)->TreeWalk = ok;
        } else {
            error = ERROR_GOPHER_DATA_ERROR;
        }
    }
    return error;
}


PRIVATE
DWORD
ParseUnknownAttribute(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferLength,
    IN DWORD NumberOfFields,
    IN DWORD FixedSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    LinePtr         -
    LineLength      -
    Buffer          -
    BufferLength    -
    NumberOfFields  -
    FixedSize       -

Return Value:

    DWORD

--*/

{
    DWORD structureSize;
    DWORD error;
    DWORD stringLength;
    LPSTR stringPtr;

    stringPtr = (LPSTR)((LPGOPHER_UNKNOWN_ATTRIBUTE_TYPE)Buffer + 1);
    ((LPGOPHER_UNKNOWN_ATTRIBUTE_TYPE)Buffer)->Text = stringPtr;
    SkipLeading(LinePtr, LineLength);
    structureSize = sizeof(GOPHER_UNKNOWN_ATTRIBUTE_TYPE)
                  + *LineLength
                  + 1
                  ;
    if (*BufferLength >= structureSize) {
        CopyString(&stringPtr, *LinePtr, *LineLength);
    } else {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    *BufferLength = structureSize;
    return error;
}


PRIVATE
DWORD
ExtractAttributeName(
    OUT LPSTR AttributeName,
    IN OUT LPDWORD AttributeNameLength,
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    AttributeName       -
    AttributeNameLength -
    LinePtr             -
    LineLength          -

Return Value:

    DWORD

--*/

{
    return ERROR_SUCCESS;
}


PRIVATE
DWORD
CharacterCount(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength,
    IN LPSTR TerminationSet
    )

/*++

Routine Description:

    Returns the number of characters in a string, up to, but not including
    the termination character. Termination character is defined as being a
    member of TerminationSet

Arguments:

    LinePtr         - IN: Pointer to string to count characters in
                      OUT: Pointer to string at character found from
                      TerminationSet

    LineLength      - IN: Current length of LinePtr
                      OUT: Remaining length of LinePtr

    TerminationSet  - Pointer to string containing characters which will
                      terminate counting

Return Value:

    DWORD
        Number of character in LinePtr, up to, but not including the
        termination character

--*/

{
    char terminationChars[256];
    int i;
    DWORD count;

    //
    // zap discovery matrix, 4 bytes at a time
    //

    for (i = 0; i < ARRAY_ELEMENTS(terminationChars); i += sizeof(DWORD)) {
        *(LPDWORD)&terminationChars[i] = 0;
    }

    //
    // for each character that we are interested in, set its matrix entry to
    // non-zero
    //

    for (i = 0; TerminationSet[i] != '\0'; ++i) {
        terminationChars[(int)TerminationSet[i]] = 1;
    }

    //
    // loop, looking for end-of-string (LineLength decremented to 0) or one
    // of the termination characters
    //

    for (count = 0; *LineLength != 0; ) {

        char ch;

        ch = *(*LinePtr)++;
        --*LineLength;
        if (terminationChars[(int)ch]) {
            break;
        }
        ++count;
    }
    return count;
}


PRIVATE
DWORD
CountCharactersToEol(
    IN OUT LPSTR* LinePtr,
    IN OUT LPDWORD LineLength
    )

/*++

Routine Description:

    Special-case version of CharacterCount - knows that the termination set
    comprised <CR>, <LF>

Arguments:

    LinePtr     - Pointer to pointer to string to count. Updated on output

    LineLength  - Pointer to length of string. Updated on output

Return Value:

    DWORD
        Length of string

--*/

{
    int i;
    DWORD count;

    //
    // loop, looking for end-of-string (LineLength decremented to 0) or one
    // of the termination characters
    //

    for (count = 0; *LineLength != 0; ) {

        char ch;

        ch = *(*LinePtr)++;
        --*LineLength;
        if ((ch == '\r') || (ch == '\n')) {
            break;
        }
        ++count;
    }
    return count;
}


PRIVATE
BOOL
SkipLeading(
    IN OUT LPSTR* String,
    IN OUT LPDWORD Length
    )
{
    while (((**String == ' ') || (**String == ':')) && (*Length != 0)) {
        ++*String;
        --*Length;
    }
    return (BOOL)(*Length != 0);
}


PRIVATE
VOID
CopyString(
    IN OUT LPSTR* String,
    IN LPSTR Source,
    IN DWORD Length
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    String  -
    Source  -
    Length  -

Return Value:

    None.

--*/

{
    memcpy(*String, Source, Length);
    *String += Length;
    *((*String)++) = '\0';
}

#endif // defined(GOPHER_ATTRIBUTE_SUPPORT)
