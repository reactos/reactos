/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapiu.h

Abstract:

    Contains prototypes etc. for gfrapiu.c

Author:

    Richard L Firth (rfirth) 19-Nov-1994

Revision History:

    19-Nov-1994
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

DWORD
TestLocatorType(
    IN LPCSTR Locator,
    IN DWORD TypeMask
    );

DWORD
GetAttributes(
    IN GOPHER_ATTRIBUTE_ENUMERATOR Enumerator,
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN LPCSTR AttributeName,
    IN LPSTR InBuffer,
    IN DWORD InBufferLength,
    OUT LPBYTE OutBuffer,
    IN DWORD OutBufferLength,
    OUT LPDWORD CharactersReturned
    );

LPSTR
MakeAttributeRequest(
    IN LPSTR Selector,
    IN LPSTR Attribute
    );

#if defined(__cplusplus)
}
#endif
