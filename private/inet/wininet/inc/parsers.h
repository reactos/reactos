/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    parsers.h

Abstract:

    Contains prototypes etc. for common\parsers.cxx

Author:

    Richard L Firth (rfirth) 03-Jul-1996

Revision History:

    03-Jul-1996 rfirth
        Created

--*/

//
// prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

BOOL
ExtractWord(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPWORD ConvertedNumber
    );

BOOL
ExtractDword(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPDWORD ConvertedNumber
    );

BOOL
ExtractInt(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPINT ConvertedNumber
    );

BOOL
SkipWhitespace(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    );

BOOL
SkipSpaces(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    );

BOOL
SkipLine(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    );

BOOL
FindToken(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    );

BOOL
FindTokenSpecial(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    );

LPSTR
NiceNum(
    OUT LPSTR Buffer,
    IN SIZE_T Number,
    IN int FieldWidth
    );

#if defined(__cplusplus)
}
#endif
