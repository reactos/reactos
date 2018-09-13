/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    icstring.hxx

Abstract:

    Contains ICSTRING class implementation. Lightweight version of familiar
    counted string class, used mainly to avoid calling strxxx functions.

    Private to WININET project.

    The ICSTRING class has the notion of copies, or references to a string.
    When an ICSTRING is created, we allocate a buffer for the string. Subsequent
    assignment (=) and augmentation (+=) operations can be made against this
    buffer, and possibly reallocate it, changing its address.

    As soon as an ICSTRING is copied, we disallow any modifications to the
    buffer pointer, since the copied object will get the old pointer value.

    ICSTRINGs can also be used to point to a substring in a larger buffer. In
    such a case, Reference will be TRUE. If the buffer pointed at is one that
    may be reallocated and potentially moved, then we maintain offsets into the
    buffer, and have to be supplied with the current address of the buffer when
    generating the pointer. Obviously, buffers are not expected to move while
    we are using the generated pointer into it.

    If a string is an offset, then Offset will be TRUE.

    This scheme is workable for WININET because we are typically creating a
    string in one place (e.g. INTERNET_CONNECT_OBJECT_HANDLE) and because of
    the way the handle objects are derived, we make copies of the string objects
    (e.g. in HTTP_REQUEST_HANDLE_OBJECT)

    Contents:
        ICSTRING

Author:

    Richard L Firth (rfirth) 18-Dec-1995

Revision History:

    18-Dec-1995 rfirth
        Created

    05-Oct-1996 rajeevd
        Added a very simple string class: XSTRING.

--*/

//
// class implementation of XSTRING
//

class XSTRING {

    LPSTR _lpsz;

public:

    XSTRING() {
        _lpsz = NULL;
    }

    ~XSTRING() {
        Free();
     }

     void Free (void) {
        if (_lpsz) {
            FREE_MEMORY (_lpsz);
        }
        _lpsz = NULL;
     }

     LPSTR GetPtr(void) {
        return _lpsz;
     }

     LPSTR ClearPtr (void) {

        // Relinquish ownership of the pointer.

        LPSTR lpszRet = _lpsz;
        _lpsz = NULL;
        return lpszRet;
     }

     void SetPtr (LPSTR* lplpszIn) {

        // Take ownership of the pointer.

        INET_ASSERT((lplpszIn != NULL) && (*lplpszIn != '\0'));

        if (_lpsz)
            FREE_MEMORY(_lpsz);
        _lpsz = *lplpszIn;
        *lplpszIn = NULL;
     }

     BOOL SetData(LPSTR lpszIn) {

        // Make a copy of the data passed in.

        INET_ASSERT(lpszIn != NULL);

        lpszIn = NewString(lpszIn);
        if (!lpszIn)
            return FALSE;
        if (_lpsz)
            FREE_MEMORY(_lpsz);
        _lpsz = lpszIn;
        return TRUE;
     }

};

//
// class implementation
//

class ICSTRING {

protected:

    //
    // _String - pointer to string buffer. This is allocated using ResizeBuffer()
    // which makes assigning new or NULL strings easy
    //

    LPSTR _String;

    //
    // _StringLength - the strlen() of _String
    //

    WORD _StringLength;

    //
    // _BufferLength - the number of bytes in the _String buffer, Initially,
    // this will be 1 more than _StringLength
    //

    WORD _BufferLength;

    //
    // _Union - access Flags also as a DWORD for initialization
    //

    union {

        struct {

            //
            // Flags - collection of boolean flags
            //

            //
            // HaveString - TRUE when a value has been assigned to this ICSTRING
            //

            BOOL HaveString : 1;

            //
            // Reference - if TRUE then this ICSTRING object simply references
            // another - that is, its a copy, and _String shouldn't be freed in
            // the destructor
            //

            BOOL Reference  : 1;

            //
            // Referenced - if TRUE then this ICSTRING object has been copied.
            // If the string pointer is changed then the copies will be invalid
            //

            BOOL Referenced : 1;

            //
            // Offset - TRUE if the string is an offset into a moveable buffer
            //

            BOOL Offset     : 1;

            //
            // Error - TRUE if the caller should call GetLastError() to find out
            // why the operation failed
            //

            BOOL Error      : 1;

        } Flags;

        //
        // Dword - used to zap the entire contents of Flags to 0
        //

        DWORD Dword;

        //
        // There are 32 bits in this Union, and only 5 are used.
        //   its a really shame to waste it, so lets make the
        //   other parts of the DWORD availble for use by others.
        //

        struct {

            BYTE FlagsByte;
            BYTE ExtraByte1;
            BYTE ExtraByte2;
            BYTE ExtraByte3;
        } Bytes;

    } _Union;

    //
    // Initialize - initializes the ICSTRING to NULLs
    //

    VOID Initialize(VOID) {
        _String = NULL;
        _StringLength = 0;
        _BufferLength = 0;
        ZapFlags();
    }

public:

    //
    // constructors
    //

    ICSTRING() {
        Initialize();
    }

    ICSTRING(LPSTR String) {
        Initialize();
        *this = String;
    }

    //
    // copy constructor
    //

    ICSTRING(ICSTRING& String);

    //
    // destructor
    //

    ~ICSTRING();

    //
    // operators
    //

    ICSTRING& operator=(LPSTR String);
    ICSTRING& operator=(ICSTRING& String);
    VOID operator+=(LPSTR String);
    VOID operator+=(char Ch);

    //
    // member functions
    //

    VOID Clear(VOID) {
        Initialize();
    }

    //
    // flags functions
    //

    VOID ZapFlags(VOID) {
        _Union.Dword = 0;
    }

    VOID SetHaveString(BOOL Value) {
        _Union.Flags.HaveString = Value;
    }

    BOOL HaveString(VOID) const {
        return _Union.Flags.HaveString;
    }

    VOID SetReference(BOOL Value) {
        _Union.Flags.Reference = Value;
    }

    BOOL IsReference(VOID) const {
        return _Union.Flags.Reference;
    }

    VOID SetReferenced(BOOL Value) {
        _Union.Flags.Referenced = Value;
    }

    BOOL IsReferenced(VOID) const {
        return _Union.Flags.Referenced;
    }

    VOID SetOffset(BOOL Value) {
        _Union.Flags.Offset = Value;
    }

    BOOL IsOffset(VOID) const {
        return _Union.Flags.Offset;
    }

    VOID SetError(BOOL Value) {
        _Union.Flags.Error = Value;
    }

    BOOL IsError(VOID) {
        return _Union.Flags.Error;
    }

    //
    // LPSTR StringAddress(VOID)
    //
    // returns the string pointer
    //

    LPSTR StringAddress(VOID) const {

        INET_ASSERT(!IsOffset());

        return _String;
    }

    //
    // LPSTR StringAddress(LPSTR)
    //
    // returns the address of the string. The string may or may not be based
    //

    LPSTR StringAddress(LPSTR Base) const {

        //INET_ASSERT(IsOffset() ? (Base != NULL) : (Base == NULL));

        //
        // assume the caller passes in Base == NULL for non-based strings
        //

        if (IsOffset()) {
            return Base + (DWORD_PTR)_String;
        } else {
            return _String;
        }
    }

    //
    // int StringLength(VOID)
    //
    // returns strlen() of the string
    //

    int StringLength(VOID) {
        return _StringLength;
    }

    void SetLength (DWORD NewLength) {
        INET_ASSERT (NewLength <= _StringLength);
        _StringLength = (WORD) NewLength;
    }

    VOID Strncat(LPVOID Pointer, int Length);
    VOID CreateStringBuffer(LPVOID Pointer, int StringLength, int BufferLength);
    VOID CreateOffsetString(DWORD Offset, DWORD Length);
    VOID CopyTo(LPSTR Buffer);
    VOID CopyTo(LPSTR Base, LPSTR Buffer);
    VOID CopyTo(LPSTR Buffer, DWORD Length);
    VOID CopyTo(LPSTR Buffer, LPDWORD Length);

    //
    // VOID MakeCopy(LPSTR, DWORD)
    //
    // Given a pointer to a string and its length, create an ICSTRING copy of it
    //

    VOID MakeCopy(LPSTR String, DWORD Length) {
        CreateStringBuffer((LPVOID)String, (int)Length, (int)Length + 1);
    }

    //
    // VOID ResizeString(DWORD dwByteSizeToAdd) - allows us to strncat more efficently.
    //

    VOID ResizeString(DWORD dwByteSizeToAdd) {

        INET_ASSERT(dwByteSizeToAdd > 0);
        INET_ASSERT(_String != NULL);
        INET_ASSERT(!IsReference());
        INET_ASSERT(!IsReferenced());

        int newLength;

        newLength = _StringLength + dwByteSizeToAdd + 1;
        if (_BufferLength < newLength) {
            _String = (LPSTR)ResizeBuffer((HLOCAL)_String, newLength, FALSE);
            _BufferLength = (WORD)newLength;
        }
    }


    //
    // VOID MakeLowerCase(VOID)
    //
    // Convert the string to all low-case characters
    //

    VOID MakeLowerCase(VOID) {

        INET_ASSERT(_String != NULL);

        _strlwr(_String);
    }

    //
    // string comparison methods
    //

    //
    // int Strnicmp(LPSTR, LPSTR, int)
    //
    // perform strnicmp on string that may or may not be based
    //

    int Strnicmp(LPSTR Base, LPSTR String, int Length) {

        INET_ASSERT(String != NULL);
        INET_ASSERT(HaveString());
        //INET_ASSERT(IsOffset() ? (Base != NULL) : (Base == NULL));

        //
        // make sure the base is NULL if the string really isn't based
        //

        if (!IsOffset()) {
            Base = NULL;
        }
        return (Length <= _StringLength)
            ? _strnicmp(Base + (DWORD_PTR)_String, String, Length)
            : -1;
    }

    //
    // int Strnicmp(LPSTR, int)
    //
    // perform strnicmp() on unbased string. Results:
    //
    //  -1  _StringLength < Length OR _String < String
    //   0  _StringLength == Length AND _String == String
    //   1  _StringLength > Length OR _String > String
    //

    int Strnicmp(LPSTR String, int Length) {

        //
        // only compare the strings if the lengths match
        //

        return (Length == _StringLength)
            ? ::_strnicmp(_String, String, Length)
            : (_StringLength - Length);
    }

    //
    // int Stricmp(LPSTR)
    //
    // perform stricmp on unbased string
    //

    int Stricmp(LPSTR String) {

        INET_ASSERT(String != NULL);
        INET_ASSERT(HaveString());
        INET_ASSERT(!IsOffset());

        //
        // we assume that the caller supplies a NULL base if the string really
        // isn't based
        //

        int slen = lstrlen(String);

        return (slen == _StringLength)
            ? ::lstrcmpi(_String, String)
            : (_StringLength - slen);
    }

    //
    // int Strcmp(LPSTR)
    //
    // perform strcmp on unbased string. Returns only match or not-match
    //

    BOOL Strcmp(LPSTR String) {

        INET_ASSERT(String != NULL);
        INET_ASSERT(HaveString());
        INET_ASSERT(!IsOffset());

        //
        // BUGBUG - ::lstrcmp() treats www.foo-bar.com and www.foobar.com as
        //          equivalent!
        //

        int slen = lstrlen(String);

        return (slen == _StringLength)
            ? (memcmp(_String, String, _StringLength) == 0)
                ? TRUE
                : FALSE
            : FALSE;
    }
};
