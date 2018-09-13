/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    infvalue.c

Abstract:

    Externally exposed INF routines for INF value retreival and manipulation.

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


BOOL
pAToI(
	IN  PCTSTR		Field,
    OUT PINT        IntegerValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    INT Value;
    UINT c;
    BOOL Neg;
    UINT Base;
    UINT NextDigitValue;
    INT OverflowCheck;
    BOOL b;

    if(!Field) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if(*Field == TEXT('-')) {
        Neg = TRUE;
        Field++;
    } else {
        Neg = FALSE;
        if(*Field == TEXT('+')) {
            Field++;
        }
    }

    if((*Field == TEXT('0')) &&
       ((*(Field+1) == TEXT('x')) || (*(Field+1) == TEXT('X')))) {
        //
        // The number is in hexadecimal.
        //
        Base = 16;
        Field += 2;
    } else {
        //
        // The number is in decimal.
        //
        Base = 10;
    }

    for(OverflowCheck = Value = 0; *Field; Field++) {

        c = (UINT)*Field;

        if((c >= (UINT)'0') && (c <= (UINT)'9')) {
            NextDigitValue = c - (UINT)'0';
        } else if(Base == 16) {
            if((c >= (UINT)'a') && (c <= (UINT)'f')) {
                NextDigitValue = (c - (UINT)'a') + 10;
            } else if ((c >= (UINT)'A') && (c <= (UINT)'F')) {
                NextDigitValue = (c - (UINT)'A') + 10;
            } else {
                break;
            }
        } else {
            break;
        }

        Value *= Base;
        Value += NextDigitValue;

        //
        // Check for overflow.  For decimal numbers, we check to see whether the
        // new value has overflowed into the sign bit (i.e., is less than the
        // previous value.  For hexadecimal numbers, we check to make sure we
        // haven't gotten more digits than will fit in a DWORD.
        //
        if(Base == 16) {
            if(++OverflowCheck > (sizeof(INT) * 2)) {
                break;
            }
        } else {
            if(Value < OverflowCheck) {
                break;
            } else {
                OverflowCheck = Value;
            }
        }
    }

    if(*Field) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    if(Neg) {
        Value = 0-Value;
    }
    b = TRUE;
    try {
        *IntegerValue = Value;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return(b);
}


DWORD
SetupGetFieldCount(
    IN PINFCONTEXT Context
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PINF_LINE Line;
    DWORD rc;
     
    rc = NO_ERROR;

    try {
        Line = InfLineFromContext(Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        Line = NULL;
    }

    if(!Line) {
        SetLastError(rc);
        return(0);
    }

    SetLastError(rc);

    if(HASKEY(Line)) {        
        return (Line->ValueCount - 2);
    } else {
        return (ISSEARCHABLE(Line) ? 1 : Line->ValueCount);
    }
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetStringFieldA(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PSTR        ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    PCWSTR Field;
    PCSTR field;
    UINT Len;
    DWORD rc, TmpRequiredSize;

    //
    // Context could be a bogus pointer -- guard access to it.
    //
    try {
        Field = pSetupGetField(Context, FieldIndex);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(Field) {
        field = UnicodeToAnsi(Field);
        if(!field) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    } else {
        //
        // (last error already set by pSetupGetField)
        //
        return FALSE;
    }

    Len = lstrlenA(field) + 1;

    //
    // RequiredSize and ReturnBuffer could be bogus pointers;
    // guard access to them.
    //
    rc = NO_ERROR;
    try {
        if(RequiredSize) {
            *RequiredSize = Len;
        }
        if(ReturnBuffer) {
            if(ReturnBufferSize >= Len) {
                lstrcpyA(ReturnBuffer, field);
            } else {
                rc = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    MyFree(field);
    SetLastError(rc);
    return(rc == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
SetupGetStringFieldW(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(FieldIndex);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetStringField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PTSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PCTSTR Field;
    UINT Len;
    DWORD rc;

    //
    // Context could be a bogus pointer -- guard access to it.
    //
    try {
        Field = pSetupGetField(Context, FieldIndex);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!Field) {
        //
        // (last error already set by pSetupGetField)
        //
        return FALSE;
    }

    Len = lstrlen(Field) + 1;

    //
    // RequiredSize and ReturnBuffer could be bogus pointers;
    // guard access to them.
    //
    rc = NO_ERROR;
    try {
        if(RequiredSize) {
            *RequiredSize = Len;
        }
        if(ReturnBuffer) {
            if(ReturnBufferSize >= Len) {
                lstrcpy(ReturnBuffer, Field);
            } else {
                rc = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}


BOOL
SetupGetIntField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PINT        IntegerValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    PCTSTR Field;

    try {
        Field = pSetupGetField(Context,FieldIndex);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Field = NULL;
    }

    return (pAToI(Field, IntegerValue));
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetLineTextA(
    IN  PINFCONTEXT Context,          OPTIONAL
    IN  HINF        InfHandle,        OPTIONAL
    IN  PCSTR       Section,          OPTIONAL
    IN  PCSTR       Key,              OPTIONAL
    OUT PSTR        ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )
{
    INFCONTEXT context;
    BOOL b;
    UINT FieldCount;
    UINT u;
    BOOL InsufficientBuffer;
    DWORD OldSize, TmpRequiredSize;
    PCWSTR Field;
    PCSTR field;
    PCWSTR section,key;

    //
    // Set up inf context.
    //
    if(Context) {
        u = NO_ERROR;
        try {
            context = *Context;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            u = ERROR_INVALID_PARAMETER;
        }
        if(u != NO_ERROR) {
            SetLastError(u);
            return(FALSE);
        }
    } else {
        if(!InfHandle || !Section || !Key) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if(Section) {
            u = CaptureAndConvertAnsiArg(Section,&section);
            if(u != NO_ERROR) {
                SetLastError(u);
                return(FALSE);
            }
        } else {
            section = NULL;
        }

        if(Key) {
            u = CaptureAndConvertAnsiArg(Key,&key);
            if(u != NO_ERROR) {
                if(section) {
                    MyFree(section);
                }
                SetLastError(u);
                return(FALSE);
            }
        } else {
            key = NULL;
        }

        b = SetupFindFirstLine(InfHandle,section,key,&context);
        u = GetLastError();

        if(section) {
            MyFree(section);
        }
        if(key) {
            MyFree(key);
        }

        if(!b) {
            SetLastError(u);
            return FALSE;
        }
    }

    //
    // Figure out how many fields are involved.
    //
    InsufficientBuffer = FALSE;
    if(FieldCount = SetupGetFieldCount(&context)) {
        TmpRequiredSize = 0;

        for(u=0; u<FieldCount; u++) {

            Field = pSetupGetField(&context, u+1);
            MYASSERT(Field);

            field = UnicodeToAnsi(Field);
            if(!field) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return(FALSE);
            }

            OldSize = TmpRequiredSize;
            TmpRequiredSize += lstrlenA(field)+1;

            if(ReturnBuffer) {
                if(TmpRequiredSize > ReturnBufferSize) {
                    InsufficientBuffer = TRUE;
                } else {
                    //
                    // lstrcpy is safe even with bad pointers
                    // (at least on NT)
                    //
                    lstrcpyA(ReturnBuffer+OldSize,field);
                    ReturnBuffer[TmpRequiredSize - 1] = ',';
                }
            }

            MyFree(field);
        }

        //
        // 0-terminate the buffer by overwriting the final comma.
        //
        if(ReturnBuffer && !InsufficientBuffer) {
            ReturnBuffer[TmpRequiredSize - 1] = 0;
        }
    } else {
        //
        // Special case when no values -- need 1 byte for nul.
        //
        if (GetLastError() != NO_ERROR) {
            //
            // actually, something went wrong reading the data from our context...
            // bail out
            //
            return(FALSE);
        }
        TmpRequiredSize = 1;
        if(ReturnBuffer) {
            if(ReturnBufferSize) {
                *ReturnBuffer = 0;
            } else {
                InsufficientBuffer = TRUE;
            }
        }
    }

    if(RequiredSize) {
        u = NO_ERROR;
        try {
            *RequiredSize = TmpRequiredSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            u = ERROR_INVALID_PARAMETER;
        }
        if(u != NO_ERROR) {
            SetLastError(u);
            return(FALSE);
        }
    }

    if(InsufficientBuffer) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}
#else
//
// Unicode stub
//
BOOL
SetupGetLineTextW(
    IN  PINFCONTEXT Context,          OPTIONAL
    IN  HINF        InfHandle,        OPTIONAL
    IN  PCWSTR      Section,          OPTIONAL
    IN  PCWSTR      Key,              OPTIONAL
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(Section);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetLineText(
    IN  PINFCONTEXT Context,          OPTIONAL
    IN  HINF        InfHandle,        OPTIONAL
    IN  PCTSTR      Section,          OPTIONAL
    IN  PCTSTR      Key,              OPTIONAL
    OUT PTSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT PDWORD      RequiredSize      OPTIONAL
    )

/*++

Routine Description:

    This function returns the contents of a line in a compact format.
    All extraneous whitespace is removed, and multi-line values are converted
    into a single contiguous string.

    For example, consider the following extract from an INF:

    HKLM, , Foo, 1, \
    ; This is a comment
    01, 02, 03

    would be returned as:
    HKLM,,Foo,1,01,02,03

Arguments:

    Context - Supplies context for an inf line whose text is to be retreived.
        If not specified, then InfHandle, Section, and Key must be.

    InfHandle - Supplies handle of the INF file to query.
        Only used if Context is NULL.

    Section - points to a null-terminated string that specifies the section
        containing the key nameof the line whose text is to be retreived.
        (Only used if InfLineHandle is NULL.)

    Key - Points to the null-terminated string containing the key name
        whose associated string is to be retrieved. (Only used if InfLineHandle is NULL.)

    ReturnBuffer - Points to the buffer that receives the retrieved string.

    ReturnBufferSize - Specifies the size, in characters, of the buffer pointed to
        by the ReturnBuffer parameter.

    RequiredSize - Receives the actual number of characters needed for the buffer
        pointed to by the ReturnBuffer parameter. If this value is larger than the
        value specified in the ReturnBufferSize parameter, the function fails and
        the function stores no data in the buffer.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE. To get extended error information,
        call GetLastError.

--*/

{
    INFCONTEXT context;
    BOOL b;
    UINT FieldCount;
    UINT u;
    BOOL InsufficientBuffer;
    DWORD OldSize, TmpRequiredSize;
    PCTSTR Field;

    //
    // Set up inf context.
    //
    if(Context) {
        u = NO_ERROR;
        try {
            context = *Context;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            u = ERROR_INVALID_PARAMETER;
        }
        if(u != NO_ERROR) {
            SetLastError(u);
            return(FALSE);
        }
    } else {
        if(!InfHandle || !Section || !Key) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        if(!SetupFindFirstLine(InfHandle, Section, Key, &context)) {
            return FALSE;
        }
    }

    //
    // Figure out how many fields are involved.
    //
    InsufficientBuffer = FALSE;
    if(FieldCount = SetupGetFieldCount(&context)) {
        TmpRequiredSize = 0;

        for(u=0; u<FieldCount; u++) {

            Field = pSetupGetField(&context, u+1);
            MYASSERT(Field);

            OldSize = TmpRequiredSize;
            TmpRequiredSize += lstrlen(Field)+1;

            if(ReturnBuffer) {
                if(TmpRequiredSize > ReturnBufferSize) {
                    InsufficientBuffer = TRUE;
                } else {
                    //
                    // lstrcpy is safe even with bad pointers
                    // (at least on NT)
                    //
                    lstrcpy(ReturnBuffer+OldSize, Field);
                    ReturnBuffer[TmpRequiredSize - 1] = TEXT(',');
                }
            }
        }

        //
        // 0-terminate the buffer by overwriting the final comma.
        //
        if(ReturnBuffer && !InsufficientBuffer) {
            ReturnBuffer[TmpRequiredSize - 1] = TEXT('\0');
        }
    } else {
        //
        // Special case when no values -- need 1 byte for nul.
        //
        if (GetLastError() != NO_ERROR) {
            //
            // actually, something went wrong reading the data from our context...
            // bail out
            //
            return(FALSE);
        }
        TmpRequiredSize = 1;
        if(ReturnBuffer) {
            if(ReturnBufferSize) {
                *ReturnBuffer = TEXT('\0');
            } else {
                InsufficientBuffer = TRUE;
            }
        }
    }

    if(RequiredSize) {
        u = NO_ERROR;
        try {
            *RequiredSize = TmpRequiredSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            u = ERROR_INVALID_PARAMETER;
        }
        if(u != NO_ERROR) {
            SetLastError(u);
            return(FALSE);
        }
    }

    if(InsufficientBuffer) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetMultiSzFieldA(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PSTR        ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    PCTSTR Field;
    UINT FieldCount;
    UINT u;
    UINT Len;
    BOOL InsufficientBuffer;
    DWORD OldSize, TmpRequiredSize;
    DWORD rc;
    PCSTR field;

    rc = NO_ERROR;

    //
    // Disallow keys
    //
    if(FieldIndex == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Figure out how many fields are involved.
    //
    FieldCount = SetupGetFieldCount(Context);
    FieldCount -= FieldIndex - 1;
    if((INT)FieldCount < 0) {
        //
        // we might have been passed a bogus context...bail out
        //
        if (GetLastError() != NO_ERROR) {
            return(FALSE);
        }
        FieldCount = 0;
    }

    //
    // Need at least one byte for the terminating nul.
    //
    TmpRequiredSize = 1;
    InsufficientBuffer = FALSE;

    if(ReturnBuffer) {
        if(ReturnBufferSize) {
            try {
                *ReturnBuffer = 0;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                rc = ERROR_INVALID_PARAMETER;
            }
            if(rc != NO_ERROR) {
                SetLastError(rc);
                return(FALSE);
            }
        } else {
            InsufficientBuffer = TRUE;
        }
    }

    for(u=0; u<FieldCount; u++) {

        try {
            Field = pSetupGetField(Context, u+FieldIndex);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }

        MYASSERT(Field);

        field = UnicodeToAnsi(Field);
        if(!field) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if((Len = lstrlenA(field)+1) == 1) {
            //
            // Then we've encountered an empty field.  Since multi-sz lists can't contain
            // an empty string, this terminates our list.
            //
            MyFree(field);
            goto clean0;
        }

        OldSize = TmpRequiredSize;
        TmpRequiredSize += Len;

        if(ReturnBuffer) {
            if(TmpRequiredSize > ReturnBufferSize) {
                InsufficientBuffer = TRUE;
            } else {
                //
                // lstrcpy is safe with bad pointers (at least on NT)
                //
                lstrcpyA(ReturnBuffer+OldSize-1,field);
                ReturnBuffer[TmpRequiredSize - 1] = 0;
            }
        }

        MyFree(field);
    }

clean0:
    if(RequiredSize) {
        try {
            *RequiredSize = TmpRequiredSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    }

    if(InsufficientBuffer) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}
#else
//
// Unicode stub
//
BOOL
SetupGetMultiSzFieldW(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PWSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(FieldIndex);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetMultiSzField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PTSTR       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PCTSTR Field;
    UINT FieldCount;
    UINT u;
    UINT Len;
    BOOL InsufficientBuffer;
    DWORD OldSize, TmpRequiredSize;
    DWORD rc;

    rc = NO_ERROR;

    //
    // Disallow keys
    //
    if(FieldIndex == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Figure out how many fields are involved.
    //
    FieldCount = SetupGetFieldCount(Context);
    FieldCount -= FieldIndex - 1;
    if((INT)FieldCount < 0) {
        if (GetLastError() != NO_ERROR) {
            return(FALSE);
        }
        FieldCount = 0;
    }

    //
    // Need at least one byte for the terminating nul.
    //
    TmpRequiredSize = 1;
    InsufficientBuffer = FALSE;

    if(ReturnBuffer) {
        if(ReturnBufferSize) {
            try {
                *ReturnBuffer = 0;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                rc = ERROR_INVALID_PARAMETER;
            }
            if(rc != NO_ERROR) {
                SetLastError(rc);
                return(FALSE);
            }
        } else {
            InsufficientBuffer = TRUE;
        }
    }

    for(u=0; u<FieldCount; u++) {

        try {
            Field = pSetupGetField(Context, u+FieldIndex);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }

        MYASSERT(Field);

        if((Len = lstrlen(Field)+1) == 1) {
            //
            // Then we've encountered an empty field.  Since multi-sz lists can't contain
            // an empty string, this terminates our list.
            //
            goto clean0;
        }

        OldSize = TmpRequiredSize;
        TmpRequiredSize += Len;

        if(ReturnBuffer) {
            if(TmpRequiredSize > ReturnBufferSize) {
                InsufficientBuffer = TRUE;
            } else {
                //
                // lstrcpy is safe with bad pointers (at least on NT)
                //
                lstrcpy(ReturnBuffer+OldSize-1, Field);
                ReturnBuffer[TmpRequiredSize - 1] = 0;
            }
        }
    }

clean0:
    if(RequiredSize) {
        try {
            *RequiredSize = TmpRequiredSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    }

    if(InsufficientBuffer) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}


BOOL
SetupGetBinaryField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PBYTE       ReturnBuffer,     OPTIONAL
    IN  DWORD       ReturnBufferSize,
    OUT LPDWORD     RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PCTSTR Field;
    UINT FieldCount;
    UINT u;
    ULONG Value;
    BOOL Store;
    PTCHAR End;
    DWORD TmpRequiredSize;
    DWORD rc;

    rc = NO_ERROR;

    //
    // Disallow keys
    //
    if(FieldIndex == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Figure out how many fields are involved.
    //
    FieldCount = SetupGetFieldCount(Context);
    FieldCount -= FieldIndex - 1;
    if((INT)FieldCount < 0) {
        if (GetLastError() != NO_ERROR) {
            return(FALSE);
        }
        FieldCount = 0;
    }

    TmpRequiredSize = FieldCount;

    Store = (ReturnBuffer && (TmpRequiredSize <= ReturnBufferSize));

    //
    // Even though we know the required size,
    // go through the loop anyway to validate the data.
    //
    for(u=0; u<FieldCount; u++) {

        try {
            if(!(Field = pSetupGetField(Context,u+FieldIndex))) {
                rc = ERROR_INVALID_HANDLE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }

        Value = _tcstoul(Field, &End, 16);

        //
        // Only the terminating nul should have caused the conversion
        // to stop. In any other case there were non-hex digits in the string.
        // Also disallow the empty string.
        //
        if((End == Field) || *End || (Value > 255)) {
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        if(Store) {
            try {
                *ReturnBuffer++ = (UCHAR)Value;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                rc = ERROR_INVALID_PARAMETER;
            }
            if(rc != NO_ERROR) {
                SetLastError(rc);
                return(FALSE);
            }
        }
    }

    if(RequiredSize) {
        try {
            *RequiredSize = TmpRequiredSize;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    }

    if(ReturnBuffer && (TmpRequiredSize > ReturnBufferSize)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}


PINF_LINE
InfLineFromContext(
    IN PINFCONTEXT Context
    )

/*++

Routine Description:

    Given an INF context, return a pointer to the inf line structure.

Arguments:

    Context - supplies a pointer to the context structure that was filled
        in by one of the line-related INF APIs.
        No validation is performed on any value in the context structure.

Return Value:

    Pointer to the relevent inf line structure.

--*/

{
    PLOADED_INF Inf;
    PINF_SECTION Section;
    PINF_LINE Line;

    Inf = (PLOADED_INF)Context->CurrentInf;

    if(!LockInf((PLOADED_INF)Context->Inf)) {
        return(NULL);
    }

    Section = &Inf->SectionBlock[Context->Section];
    Line = &Inf->LineBlock[Section->Lines + Context->Line];

    UnlockInf((PLOADED_INF)Context->Inf);
    return(Line);
}

/////////////////////////////////////////////////////////////////
//
// Internal routines
//
/////////////////////////////////////////////////////////////////




BOOL 
pSetupGetSecurityInfo( 
    IN HINF Inf,
    IN PCTSTR SectionName, 
    OUT PCTSTR *SecDesc )
{

    BOOL b;
    PTSTR SecuritySectionName;
    INFCONTEXT LineContext;
    DWORD rc;

    
    SecuritySectionName = (PTSTR)MyMalloc( ((lstrlen(SectionName) + lstrlen((PCTSTR)L".Security"))*sizeof(TCHAR)) + 3l );
    if( !SecuritySectionName ){
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return( FALSE );
    }
              
    lstrcpy( SecuritySectionName, SectionName );
    lstrcat( SecuritySectionName, (PCTSTR)(L".Security") );
    b = SetupFindFirstLine(Inf,(PCTSTR)SecuritySectionName,NULL,&LineContext);
    MyFree( SecuritySectionName );
    if(!b)
        return( FALSE );    // Section did not exist or other error
    

    if( !(*SecDesc = pSetupGetField( &LineContext, 1 )) )
        return( FALSE );            // Error code is present by checking GetLastError() if needed
    else
        return( TRUE );


}




PCTSTR
pSetupGetField(
    IN PINFCONTEXT Context,
    IN DWORD       FieldIndex
    )

/*++

Routine Description:

    Retreive a field from a line.

Arguments:

    Context - supplies inf context. No validation is performed
        on the values contained in this structure.

    FieldIndex - supplies 1-based index of field to retreive.
        An index of 0 retreives the key, if it exists.

Return Value:

    Pointer to string. The caller must not write into this buffer.

    If the field index is not valid, the return value is NULL,
    and SetLastError() will have been called.

--*/

{
    PINF_LINE Line;
    PTSTR p = NULL;
    DWORD Err = NO_ERROR;

    //
    // InfLineFromContext does it's own INF locking, but the later call
    // to InfGetField doesn't, so go ahead and grab the lock up front.
    //
    if(LockInf((PLOADED_INF)Context->Inf)) {

        if(Line = InfLineFromContext(Context)) {

            if((p = InfGetField(Context->CurrentInf,Line,FieldIndex,NULL)) == NULL) {
                Err = ERROR_INVALID_PARAMETER;
            }

        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

        UnlockInf((PLOADED_INF)Context->Inf);

    } else {
        Err = ERROR_INVALID_HANDLE;
    }

    SetLastError(Err);
    return p;
}

BOOL
pSetupGetDriverDate(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    IN OUT PFILETIME  pFileTime
    )

/*++

Routine Description:

    Retreive the date from a specified Section.

    The Date specified in an INF section has the following format:

    DriverVer=xx/yy/zzzz

    	or

    DriverVer=xx-yy-zzzz

    where xx is the month, yy is the day, and zzzz is the for digit year.
    Note that the year MUST be 4 digits.  A year of 98 will be considered
    0098 and not 1998!

    This date should be the date of the Drivers and not for the INF itself.
    So a single INF can have multiple driver install Sections and each can
    have different dates depending on when the driver was last updated.

Arguments:

    InfHandle - Supplies handle of the INF file to query.

    Section - points to a null-terminated string that specifies the section
        of the driver to get the FILETIME infomation.

    pFileTime - points to a FILETIME structure that will receive the Date,
    	if it exists.

Return Value:

	BOOL. TRUE if a valid date existed in the specified Section and FALSE otherwise.

--*/

{
	DWORD rc;
	SYSTEMTIME SystemTime;
	INFCONTEXT InfContext;
	TCHAR DriverDate[20];
	PTSTR Convert, Temp;
	DWORD Value;
	
	rc = NO_ERROR;

    try {

		*DriverDate = 0;
		ZeroMemory(&SystemTime, sizeof(SYSTEMTIME));
		pFileTime->dwLowDateTime = 0;
		pFileTime->dwHighDateTime = 0;

	    if(SetupFindFirstLine(InfHandle, Section, pszDriverVer, &InfContext)) {

            if ((SetupGetStringField(&InfContext,
                                1,
                                DriverDate,
                                sizeof(DriverDate),
                                NULL)) &&
              	 (*DriverDate)) {

				Convert = DriverDate;

				if (*Convert) {

					Temp = DriverDate;
					while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
						Temp++;
				
					*Temp = 0;							         

					//
					//Convert the month
					//
					pAToI(Convert, (PINT)&Value);
					SystemTime.wMonth = LOWORD(Value);

					Convert = Temp+1;

					if (*Convert) {

						Temp = Convert;
						while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
							Temp++;
					
						*Temp = 0;							         

						//
						//Convert the day
						//
						pAToI(Convert, (PINT)&Value);
						SystemTime.wDay = LOWORD(Value);

						Convert = Temp+1;

						if (*Convert) {

							//
							//Convert the year
							//
							pAToI(Convert, (PINT)&Value);
							SystemTime.wYear = LOWORD(Value);

							//
							//Convert SYSTEMTIME into FILETIME
							//
							SystemTimeToFileTime(&SystemTime, pFileTime);
						}
					}						
				}
			}              	 

	    }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        SetLastError(rc);
        return FALSE;
    }

	SetLastError(NO_ERROR);
    return((pFileTime->dwLowDateTime != 0) || (pFileTime->dwHighDateTime != 0));
}



BOOL
pSetupGetDriverVersion(
    IN  HINF        InfHandle,
    IN  PCTSTR      Section,
    OUT DWORDLONG   *Version
    )

/*++

Routine Description:

    Retreive the driver version from a specified Section.

    The driver version specified in an INF section has the following format:

    DriverVer=xx/yy/zzzz, a.b.c.d

    	or

    DriverVer=xx-yy-zzzz, a.b.c.d

	a.b.c.d is the version of the driver, where a, b, c, and d are all WORD
	decimal values.

	The version is in the second field in the DriverVer INF value, the driver date
	is in the first field.

Arguments:

    InfHandle - Supplies handle of the INF file to query.

    Section - points to a null-terminated string that specifies the section
        of the driver to get the FILETIME infomation.

    Version - points to a DWORDLONG value that will receive the version,
    	if it exists.

Return Value:

	BOOL. TRUE if a valid driver version existed in the specified Section and FALSE otherwise.

--*/

{
	DWORD rc;
	INFCONTEXT InfContext;
	TCHAR DriverVersion[LINE_LEN];
	BOOL bEnd = FALSE;
	INT MajorHiWord, MajorLoWord, MinorHiWord, MinorLoWord;
	PTSTR Convert, Temp;

	rc = NO_ERROR;

    try {

        *DriverVersion = 0;
		*Version = 0;
		MajorHiWord = MajorLoWord = MinorHiWord = MinorLoWord = 0;

	    if(SetupFindFirstLine(InfHandle, Section, pszDriverVer, &InfContext)) {

            if ((SetupGetStringField(&InfContext,
                                2,
                                DriverVersion,
                                sizeof(DriverVersion),
                                NULL)) &&
              	 (*DriverVersion)) {

				Convert = DriverVersion;

				if (*Convert) {

					Temp = DriverVersion;
					while (*Temp && (*Temp != TEXT('.'))) {
					
						Temp++;
					}

                    if (!*Temp) {

                       bEnd = TRUE;
                    }
				
					*Temp = 0;							         

					//
					//Convert the HIWORD of the major version
					//
					if (pAToI(Convert, (PINT)&MajorHiWord)) {

    					Convert = Temp+1;

    					if (!bEnd && *Convert) {

    						Temp = Convert;
    						while (*Temp && (*Temp != TEXT('.'))) {
    						
    							Temp++;
    						}

                            if (!*Temp) {

                                bEnd = TRUE;
                            }

    						*Temp = 0;

    						//
    						//Convert the LOWORD of the major version
    						//
    						if (pAToI(Convert, (PINT)&MajorLoWord)) {

        						Convert = Temp+1;

        						if (!bEnd && *Convert) {

        							Temp = Convert;
        							while (*Temp && (*Temp != TEXT('.'))) {
        							
        								Temp++;
        							}

                                    if (!*Temp) {
                                        
                                        bEnd = TRUE;
                                    }
                                    
        							*Temp = 0;

        							//
        							//Convert the HIWORD of the minor version
        							//
        							if (pAToI(Convert, (PINT)&MinorHiWord)) {

            							Convert = Temp+1;

            							if (!bEnd && *Convert) {

            								Temp = Convert;
            								while (*Temp && (*Temp != TEXT('.'))) {
            								
            									Temp++;
            								}

            								*Temp = 0;

            								//
            								//Convert the LOWORD of the minor version
            								//
            								pAToI(Convert, (PINT)&MinorLoWord);
            							}
            						}
        						}
        					}
    					}
					}


					*Version = (((DWORDLONG)MajorHiWord << 48) +
							     ((DWORDLONG)MajorLoWord << 32) +
								 ((DWORDLONG)MinorHiWord << 16) +
								  (DWORDLONG)MinorLoWord);
				}
			}              	 

	    }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        *Version = 0;
        SetLastError(rc);
        return FALSE;
    }

	SetLastError(NO_ERROR);
    return(*Version != 0);
}

