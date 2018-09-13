/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    parse.h

Abstract:

    Manifests, macros, types and prototypes for parse.c

Author:

    Richard L Firth (rfirth) 17-Oct-1994

Revision History:

    17-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

BOOL
IsValidLocator(
    IN LPCSTR Locator,
    IN DWORD MaximumLength
    );

BOOL
IsGopherPlus(
    IN LPCSTR Locator
    );

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
    );

DWORD
GopherCharToType(
    IN CHAR GopherChar
    );

CHAR
GopherTypeToChar(
    IN DWORD Attribute
    );

DWORD
GetDirEntry(
    IN LPVIEW_INFO ViewInfo,
    OUT LPGOPHER_FIND_DATA FindData
    );

DWORD
ReadData(
    IN LPVIEW_INFO ViewInfo,
    OUT LPDWORD BytesReturned
    );

BOOL
CopyToEol(
    IN OUT LPSTR* Destination,
    IN OUT LPDWORD DestinationLength,
    IN OUT LPSTR* Source,
    IN OUT LPDWORD SourceLength
    );

DWORD
IsGopherPlusToken(
    IN LPSTR Token,
    IN DWORD TokenLength,
    IN LPSTR Buffer,
    IN DWORD BufferLength
    );

DWORD
MapAttributeNameToId(
    IN LPCSTR AttributeName
    );

VOID
MapAttributeToIds(
    IN LPCSTR AttributeName,
    OUT LPDWORD CategoryId,
    OUT LPDWORD AttributeId
    );

BOOL
MapAttributeIdToNames(
    IN DWORD AttributeId,
    OUT LPSTR* CategoryName,
    OUT LPSTR* AttributeName
    );

DWORD
GetGopherNumber(
    IN OUT LPSTR* pString
    );

BOOL
ExtractDateAndTime(
    IN OUT LPSTR* pString,
    OUT LPFILETIME pFileTime
    );

BOOL
ExtractView(
    IN OUT LPSTR* pString,
    OUT LPSTR ContentType,
    IN OUT LPDWORD ContentTypeLength,
    OUT LPSTR Language,
    IN OUT LPDWORD LanguageLength,
    OUT LPDWORD Size
    );

BOOL
FindAttribute(
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN LPCSTR AttributeName,
    IN OUT LPSTR* Buffer,
    IN OUT LPDWORD BufferLength
    );

VOID
FindNextAttribute(
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN OUT LPSTR* Buffer,
    IN OUT LPDWORD BufferLength
    );

DWORD
EnumerateAttribute(
    IN GOPHER_ATTRIBUTE_ENUMERATOR Enumerator,
    IN LPSTR LinePtr,
    IN DWORD LineLength,
    IN LPBYTE Buffer,
    IN DWORD BufferLength,
    OUT LPBOOL ResumeEnumeration
    );

#if defined(__cplusplus)
}
#endif
