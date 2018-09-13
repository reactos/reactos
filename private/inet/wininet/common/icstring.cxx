/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    icstring.hxx

Abstract:

    Contains ICSTRING class methods. Split from ICSTRING.HXX (inline methods)

    Contents:
        ICSTRING::ICSTRING(ICSTRING&)
        ICSTRING::~ICSTRING()
        ICSTRING::operator=(LPSTR)
        ICSTRING::operator=(ICSTRING&)
        ICSTRING::operator+=(LPSTR)
        ICSTRING::operator+=(char)
        ICSTRING::strncat(LPVOID, int)
        ICSTRING::CreateStringBuffer(LPVOID, int, int)
        ICSTRING::CreateOffsetString(DWORD, DWORD)
        ICSTRING::CopyTo(LPSTR)
        ICSTRING::CopyTo(LPSTR, LPSTR)
        ICSTRING::CopyTo(LPSTR, DWORD)
        ICSTRING::CopyTo(LPSTR, LPDWORD)

Author:

    Richard L Firth (rfirth) 18-Dec-1995

Revision History:

    18-Dec-1995 rfirth
        Created

--*/

#include <wininetp.h>

//
// methods
//

//
//ICSTRING::ICSTRING(
//    IN ICSTRING& String
//    )
//
///*++
//
//Routine Description:
//
//    copy constructor. We now create an entirely new string (used to be just a
//    reference of the rvalue)
//
//Arguments:
//
//    String  - to copy
//
//Return Value:
//
//    None.
//
//--*/
//
//{
//    //
//    // can't already have string in lvalue, & rvalue mustn't be an offset string
//    //
//
//    INET_ASSERT(!HaveString());
//    INET_ASSERT(!String.IsOffset());
//    INET_ASSERT(!String.IsError());
//
//    *this = String.StringAddress();
//}


ICSTRING::~ICSTRING(
    VOID
    )

/*++

Routine Description:

    destructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // can only free the string if it is not a reference. Offset type
    // implies reference
    //

    if (!IsReference() && (_String != NULL)) {

        INET_ASSERT(!IsOffset());

        (VOID)ResizeBuffer((HLOCAL)_String, 0, FALSE);
    }
}


ICSTRING&
ICSTRING::operator=(
    IN LPSTR String
    )

/*++

Routine Description:

    Copy/assigment. Copies a string to this object. If NULL, frees up the
    current buffer

Arguments:

    String  - to be assigned

Return Value:

    ICSTRING&

--*/

{
    //
    // if this is an offset string then there's not much to do
    //

    if (IsOffset()) {
        Initialize();
        return *this;
    }

    //
    // string MUST NOT be a copy (we'll free the real string pointer, owned
    // by another object) and SHOULD NOT be copied itself (the objects with
    // copies of the string will potentially have a bad pointer)
    //

    INET_ASSERT((String == NULL) ? TRUE : !IsReference());
    INET_ASSERT(!IsReferenced());

    //
    // if the pointer is NULL or the string is empty then we are freeing the
    // string pointer
    //

    int len;

    if (String == NULL) {
        len = 0;
    } else {
        len = ::strlen(String);
        if (len != 0) {
            ++len;
        }
    }

    //
    // free or grow the buffer, depending on requirements
    //

    if ((len > _BufferLength) || ((len == 0) && (_String != NULL))) {
        _String = (LPSTR)ResizeBuffer((HLOCAL)_String, len, FALSE);
        _BufferLength = (_String != NULL) ? len : 0;
    }
    if (_String != NULL) {

        INET_ASSERT(len != 0);

        memcpy((LPVOID)_String, (LPVOID)String, len);
        _StringLength = len - 1;
        SetHaveString(TRUE);
        SetError(FALSE);
    } else {
        _StringLength = 0;
        SetHaveString(FALSE);
        SetError(len != 0); // an error might have occurred
    }
    return *this;
}


ICSTRING&
ICSTRING::operator=(
    IN ICSTRING& String
    )

/*++

Routine Description:

    Copy/assignment. Makes new copy of object (used to just make a reference)

Arguments:

    String  - ICSTRING object to be assigned

Return Value:

    ICSTRING&

--*/

{
    INET_ASSERT(!IsReferenced());
    INET_ASSERT(!String.IsReferenced());
    INET_ASSERT(!String.IsError());

    //
    // if we're copying an offset, make sure that any string we may already
    // have is freed
    //

    if (String.IsOffset()) {
        if (_String != NULL) {
            ResizeBuffer(_String, 0, FALSE);
        }
        _String = String._String;
        _StringLength = String._StringLength;
        _BufferLength = String._BufferLength;
        _Union.Dword = String._Union.Dword;
    } else {

        INET_ASSERT(!IsOffset() && !String.IsOffset());

        //
        // use string assignment to correctly setup this object
        //

        *this = String.StringAddress();
    }
    return *this;
}


VOID
ICSTRING::operator+=(
    IN LPSTR String
    )

/*++

Routine Description:

    Concatenates a string to the buffer. Reallocates it if necessary. String
    CANNOT be NULL

Arguments:

    String  - to concatenate

Return Value:

    None.

--*/

{
    INET_ASSERT(!IsReference());
    INET_ASSERT(!IsReferenced());
    INET_ASSERT(!IsOffset());
    INET_ASSERT(String != NULL);

    if (IsError()) {
        return;
    }

    if (*String == '\0') {
        return;
    }

    int len = ::strlen(String);
    int newlen = _StringLength + len + 1;

    if (_BufferLength < newlen) {
        _String = (LPSTR)ResizeBuffer((HLOCAL)_String, newlen, FALSE);

        INET_ASSERT((_String == NULL) ? (newlen == 0) : TRUE);

        _BufferLength = (unsigned short) newlen;
    }
    if (_String != NULL) {
        memcpy((LPVOID)((LPBYTE)_String + _StringLength),
               (LPVOID)String,
               len + 1
               );
        _StringLength += (unsigned short) len;
    } else {
        _StringLength = 0;
        _BufferLength = 0;
        SetError(TRUE);
    }
}


VOID
ICSTRING::operator+=(
    IN char Ch
    )

/*++

Routine Description:

    Concatenates a character to the buffer. Reallocates it if necessary. Ch
    CAN be '\0'

Arguments:

    Ch  - to concatenate

Return Value:

    None.

--*/

{
    INET_ASSERT(!IsReference());
    INET_ASSERT(!IsReferenced());
    INET_ASSERT(!IsOffset());

    if (IsError()) {
        return;
    }

    int newlen = _StringLength + 2;

    if (_BufferLength < newlen) {
        _String = (LPSTR)ResizeBuffer((HLOCAL)_String, newlen, FALSE);

        INET_ASSERT((_String == NULL) ? (newlen == 0) : TRUE);

        _BufferLength = (unsigned short) newlen;
    }
    if (_String != NULL) {
        _String[_StringLength] = Ch;
        ++_StringLength;
        _String[_StringLength] = '\0';
    } else {
        _StringLength = 0;
        _BufferLength = 0;
        SetError(TRUE);
    }
}


VOID
ICSTRING::Strncat(
    IN LPVOID Pointer,
    IN int Length
    )

/*++

Routine Description:

    Copies Length characters from Pointer to the end of _String

Arguments:

    Pointer - place to copy from

    Length  - number of characters to copy

Return Value:

    None.

--*/

{
    if (IsError()) {
        return;
    }

    INET_ASSERT(Pointer != NULL);
    INET_ASSERT(Length != 0);
    INET_ASSERT(_String != NULL);
    INET_ASSERT(!IsReference());
    INET_ASSERT(!IsReferenced());

    int newLength;

    newLength = _StringLength + Length + 1;
    if (_BufferLength < newLength) {
        _String = (LPSTR)ResizeBuffer((HLOCAL)_String, newLength, FALSE);
        _BufferLength = (unsigned short) newLength;
    }
    if (_String != NULL) {
        memcpy((LPVOID)&_String[_StringLength], Pointer, Length);
        _StringLength += (unsigned short) Length;

        INET_ASSERT(_StringLength < _BufferLength);

        _String[_StringLength] = '\0';
    } else {
        _StringLength = 0;
        _BufferLength = 0;
        SetError(TRUE);
    }
}


VOID
ICSTRING::CreateStringBuffer(
    IN LPVOID Pointer,
    IN int StringLength,
    IN int BufferLength
    )

/*++

Routine Description:

    In order to avoid reallocations, if we know the size of the buffer we
    want for several strcat()'s, e.g., we can allocate it once, copy the
    initial string here, then perform multiple concatenate operations (+=)

Arguments:

    Pointer         - place to start copying from

    StringLength    - length of string

    BufferLength    - length of buffer required

Return Value:

    None.

--*/

{
    INET_ASSERT(Pointer != NULL);
    INET_ASSERT(BufferLength > StringLength);
    INET_ASSERT(BufferLength != 0);

    //
    // if we currently have an offset string then initialize to a non-offset
    //

    if (IsOffset()) {
        Initialize();
    }
    _String = (LPSTR)ResizeBuffer(_String, BufferLength, FALSE);
    if (_String != NULL) {
        _StringLength = (unsigned short) StringLength;
        _BufferLength = (unsigned short) BufferLength;
        memcpy((LPVOID)_String, Pointer, _StringLength);
        _String[_StringLength] = '\0';
        SetHaveString(TRUE);
        SetReference(FALSE);
        SetReferenced(FALSE);
        SetOffset(FALSE);
        SetError(FALSE);
    } else {
        SetError(TRUE);
    }
}


VOID
ICSTRING::CreateOffsetString(
    IN DWORD Offset,
    IN DWORD Length
    )

/*++

Routine Description:

    Create a reference ICSTRING that is an offset within another buffer

Arguments:

    Offset  - offset into buffer

    Length  - of string

Return Value:

    None.

--*/

{
    INET_ASSERT(Length < 0xffff);

    _String = (LPSTR)Offset;
    _StringLength = (WORD)Length;
    _BufferLength = (WORD)Length;
    ZapFlags();
    SetHaveString(TRUE);    // ICSTRING initialized with non-NULL
    SetReference(TRUE);     // reference to another string buffer
    SetOffset(TRUE);        // offset from base
    SetError(FALSE);
}


VOID
ICSTRING::CopyTo(
    IN LPSTR Buffer
    )

/*++

Routine Description:

    Copies source _String to destination Buffer

Arguments:

    Buffer  - place to copy to

Return Value:

    None.

--*/

{
    INET_ASSERT(Buffer != NULL);
    INET_ASSERT(!IsOffset());

    memcpy((LPVOID)Buffer, (LPVOID)_String, _StringLength);
    Buffer[_StringLength] = '\0';
}


VOID
ICSTRING::CopyTo(
    IN LPSTR Base,
    IN LPSTR Buffer
    )

/*++

Routine Description:

    Copies a based (offset) string from source Base + _String to destination
    Buffer

Arguments:

    Base    - value for base

    Buffer  - place to write string

Return Value:

    None.

--*/

{
    INET_ASSERT(Buffer != NULL);
    //INET_ASSERT(IsOffset() ? (Base != NULL) : (Base == NULL));

    memcpy((LPVOID)Buffer,
           IsOffset() ? (Base + (DWORD_PTR)_String) : _String,
           _StringLength
           );
    Buffer[_StringLength] = '\0';
}


VOID
ICSTRING::CopyTo(
    IN LPSTR Buffer,
    IN DWORD Length
    )

/*++

Routine Description:

    Copies at most Length characters from source _String to destination
    Buffer

Arguments:

    Buffer  - place to write string

    Length  - number of characters to copy

Return Value:

    None.

--*/

{
    INET_ASSERT(Buffer != NULL);
    INET_ASSERT(!IsOffset());

    DWORD length = min(Length - 1, _StringLength);

    memcpy((LPVOID)Buffer, (LPVOID)_String, length);
    Buffer[length] = '\0';
}


VOID
ICSTRING::CopyTo(
    IN LPSTR Buffer,
    IN OUT LPDWORD Length
    )

/*++

Routine Description:

    Copies at most *Length characters from source _String to destination
    Buffer. Updates *Length to be number of characters copied, not including
    terminating NUL

Arguments:

    Buffer  - place to write string

    Length  - IN: length of buffer
              OUT: number of characters copied

Return Value:

    None.

--*/

{
    INET_ASSERT(Buffer != NULL);
    INET_ASSERT(!IsOffset());

    DWORD length = min(*Length - 1, _StringLength);

    memcpy((LPVOID)Buffer, (LPVOID)_String, length);
    Buffer[length] = '\0';
    *Length = length;
}
