/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapip.h

Abstract:

    Contains private or internal definitions for client gopher protocol
    provider

Author:

    Richard L Firth (rfirth) 13-Oct-1994

Revision History:

    13-Oct-1994 rfirth
        Created

--*/

//
// manifests
//

//
// gopher strings
//

#define GOPHER_REQUEST_TERMINATOR               "\r\n"
#define GOPHER_REQUEST_TERMINATOR_LENGTH        (sizeof(GOPHER_REQUEST_TERMINATOR) - 1)
#define GOPHER_DOT_TERMINATOR                   ".\r\n"
#define GOPHER_DOT_TERMINATOR_LENGTH            (sizeof(GOPHER_DOT_TERMINATOR) - 1)

//
// gopher+ strings
//

#define GOPHER_PLUS_INDICATOR                   "\t+"
#define GOPHER_PLUS_INDICATOR_LENGTH            (sizeof(GOPHER_PLUS_INDICATOR) - 1)
#define GOPHER_PLUS_ITEM_INFO                   "\t!"
#define GOPHER_PLUS_ITEM_INFO_LENGTH            (sizeof(GOPHER_PLUS_ITEM_INFO) - 1)
#define GOPHER_PLUS_INFO_REQUEST                "\t!\r\n"
#define GOPHER_PLUS_INFO_REQUEST_LENGTH         (sizeof(GOPHER_PLUS_INFO_REQUEST) - 1)
#define GOPHER_PLUS_DIRECTORY_REQUEST           "\t$"
#define GOPHER_PLUS_DIRECTORY_REQUEST_LENGTH    (sizeof(GOPHER_PLUS_DIRECTORY_REQUEST) - 1)
#define GOPHER_PLUS_INFO_TOKEN                  "+INFO"
#define GOPHER_PLUS_INFO_TOKEN_LENGTH           (sizeof(GOPHER_PLUS_INFO_TOKEN) - 1)
#define GOPHER_PLUS_ADMIN_TOKEN                 "+ADMIN"
#define GOPHER_PLUS_ADMIN_TOKEN_LENGTH          (sizeof(GOPHER_PLUS_ADMIN_TOKEN) - 1)
#define GOPHER_PLUS_VIEWS_TOKEN                 "+VIEWS"
#define GOPHER_PLUS_VIEWS_TOKEN_LENGTH          (sizeof(GOPHER_PLUS_VIEWS_TOKEN) - 1)

//
// single characters
//

#define GOPHER_FIELD_SEPARATOR          '\t'
#define GOPHER_PLUS_SUCCESS_INDICATOR   '+'
#define GOPHER_PLUS_ERROR_INDICATOR     '-'

//
// misc.
//

#define INVALID_GOPHER_CHAR             0
#define INVALID_GOPHER_TYPE             0
#define DEFAULT_GOPHER_DISPLAY_STRING   ""
#define DEFAULT_GOPHER_SELECTOR_STRING  ""
#define UNKNOWN_GOPHER_TYPE             0

//
// types and masks
//

#define VALID_GOPHER_TYPES              (GOPHER_TYPE_TEXT_FILE          \
                                        | GOPHER_TYPE_DIRECTORY         \
                                        | GOPHER_TYPE_CSO               \
                                        | GOPHER_TYPE_MAC_BINHEX        \
                                        | GOPHER_TYPE_DOS_ARCHIVE       \
                                        | GOPHER_TYPE_UNIX_UUENCODED    \
                                        | GOPHER_TYPE_INDEX_SERVER      \
                                        | GOPHER_TYPE_TELNET            \
                                        | GOPHER_TYPE_BINARY            \
                                        | GOPHER_TYPE_REDUNDANT         \
                                        | GOPHER_TYPE_TN3270            \
                                        | GOPHER_TYPE_GIF               \
                                        | GOPHER_TYPE_IMAGE             \
                                        | GOPHER_TYPE_BITMAP            \
                                        | GOPHER_TYPE_MOVIE             \
                                        | GOPHER_TYPE_SOUND             \
                                        | GOPHER_TYPE_HTML              \
                                        | GOPHER_TYPE_PDF               \
                                        | GOPHER_TYPE_CALENDAR          \
                                        | GOPHER_TYPE_INLINE            \
                                        )

#define GOPHER_TYPE_MASK                (~(GOPHER_TYPE_GOPHER_PLUS))

#define GOPHER_ATTRIBUTE_MASK           (GOPHER_TYPE_GOPHER_PLUS)

#define GOPHER_FILE_MASK                (GOPHER_TYPE_TEXT_FILE          \
                                        | GOPHER_TYPE_MAC_BINHEX        \
                                        | GOPHER_TYPE_DOS_ARCHIVE       \
                                        | GOPHER_TYPE_UNIX_UUENCODED    \
                                        | GOPHER_TYPE_BINARY            \
                                        | GOPHER_TYPE_GIF               \
                                        | GOPHER_TYPE_IMAGE             \
                                        | GOPHER_TYPE_BITMAP            \
                                        | GOPHER_TYPE_MOVIE             \
                                        | GOPHER_TYPE_SOUND             \
                                        | GOPHER_TYPE_HTML              \
                                        | GOPHER_TYPE_PDF               \
                                        | GOPHER_TYPE_CALENDAR          \
                                        | GOPHER_TYPE_INLINE            \
                                        )

#define GOPHER_DIRECTORY_MASK           (GOPHER_TYPE_DIRECTORY)


#define GOPHER_DOT_TERMINATED_TYPES     (GOPHER_TYPE_TEXT_FILE          \
                                        | GOPHER_TYPE_DIRECTORY         \
                                        | GOPHER_TYPE_MAC_BINHEX        \
                                        | GOPHER_TYPE_UNIX_UUENCODED    \
                                        | GOPHER_TYPE_INDEX_SERVER      \
                                        )

//
// gopher locator characters
//

#define GOPHER_CHAR_REDUNDANT           '+'
#define GOPHER_CHAR_TEXT_FILE           '0'
#define GOPHER_CHAR_DIRECTORY           '1'
#define GOPHER_CHAR_CSO                 '2'
#define GOPHER_CHAR_ERROR               '3'
#define GOPHER_CHAR_MAC_BINHEX          '4'
#define GOPHER_CHAR_DOS_ARCHIVE         '5'
#define GOPHER_CHAR_UNIX_UUENCODED      '6'
#define GOPHER_CHAR_INDEX_SERVER        '7'
#define GOPHER_CHAR_TELNET              '8'
#define GOPHER_CHAR_BINARY              '9'
#define GOPHER_CHAR_BITMAP              ':'
#define GOPHER_CHAR_MOVIE               ';'
#define GOPHER_CHAR_SOUND               '<'
#define GOPHER_CHAR_IMAGE               'I'
//#define GOPHER_CHAR_BIG_M               'M'
#define GOPHER_CHAR_PDF                 'P'
#define GOPHER_CHAR_TN3270              'T'
#define GOPHER_CHAR_CALENDAR            'c'
//#define GOPHER_CHAR_LITTLE_E            'e'
#define GOPHER_CHAR_GIF                 'g'
#define GOPHER_CHAR_INLINE              'i'
#define GOPHER_CHAR_HTML                'h'
#define GOPHER_CHAR_SOUND_2             's'

//
// macros
//

#define IS_DOT_TERMINATED_REQUEST(request) \
    ((request & GOPHER_DOT_TERMINATED_TYPES) ? TRUE : FALSE)
