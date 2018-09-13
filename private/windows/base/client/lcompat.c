/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lcompat.c

Abstract:

    This module implements the _l and l compatability functions
    like _lread, lstrlen...

Author:

    Mark Lucovsky (markl) 13-Mar-1991

Revision History:

--*/

#include "basedll.h"

int
WINAPI
_lopen(
    LPCSTR lpPathName,
    int iReadWrite
    )
{

    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);
    //
    // Compute Desired Access
    //

    if ( iReadWrite & OF_WRITE ) {
        DesiredAccess = GENERIC_WRITE;
        }
    else {
        DesiredAccess = GENERIC_READ;
        }
    if ( iReadWrite & OF_READWRITE ) {
        DesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
        }

    //
    // Compute ShareMode
    //

    ShareMode = BasepOfShareToWin32Share((DWORD)iReadWrite);

    CreateDisposition = OPEN_EXISTING;

    //
    // Open the file
    //

    hFile = CreateFile(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                0,
                NULL
                );

    return (HFILE)HandleToUlong(hFile);
}

HFILE
WINAPI
_lcreat(
    LPCSTR lpPathName,
    int  iAttribute
    )
{
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD CreateDisposition;

    SetLastError(0);

    //
    // Compute Desired Access
    //

    DesiredAccess = (GENERIC_READ | GENERIC_WRITE);

    ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;;

    //
    // Compute Create Disposition
    //

    CreateDisposition = CREATE_ALWAYS;

    //
    // Open the file
    //

    hFile = CreateFile(
                lpPathName,
                DesiredAccess,
                ShareMode,
                NULL,
                CreateDisposition,
                iAttribute & FILE_ATTRIBUTE_VALID_FLAGS,
                NULL
                );

    return (HFILE)HandleToUlong(hFile);
}

UINT
WINAPI
_lread(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT uBytes
    )
{
    DWORD BytesRead;
    BOOL b;

    b = ReadFile((HANDLE)IntToPtr(hFile),lpBuffer,(DWORD)uBytes,&BytesRead,NULL);
    if ( b ) {
        return BytesRead;
        }
    else {
        return (DWORD)0xffffffff;
        }
}


UINT
WINAPI
_lwrite(
    HFILE hFile,
    LPCSTR lpBuffer,
    UINT uBytes
    )
{
    DWORD BytesWritten;
    BOOL b;

    if ( uBytes ) {
        b = WriteFile((HANDLE)IntToPtr(hFile),(CONST VOID *)lpBuffer,(DWORD)uBytes,&BytesWritten,NULL);
        }
    else {
        BytesWritten = 0;
        b = SetEndOfFile((HANDLE)IntToPtr(hFile));
        }

    if ( b ) {
        return BytesWritten;
        }
    else {
        return (DWORD)0xffffffff;
        }
}

HFILE
WINAPI
_lclose(
    HFILE hFile
    )
{
    BOOL b;

    b = CloseHandle((HANDLE)IntToPtr(hFile));
    if ( b ) {
        return (HFILE)0;
        }
    else {
        return (HFILE)-1;
        }
}

LONG
WINAPI
_llseek(
    HFILE hFile,
    LONG lOffset,
    int iOrigin
    )
{
    DWORD SeekType;

    switch ( iOrigin ) {
        case 0:
            SeekType = FILE_BEGIN;
            break;
        case 1:
            SeekType = FILE_CURRENT;
            break;
        case 2:
            SeekType = FILE_END;
            break;
        default:
            return -1;
            }

    return (int)SetFilePointer((HANDLE)IntToPtr(hFile), lOffset, NULL, SeekType);
}

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) || defined(_IA64_)

int
WINAPI
MulDiv (
    int nNumber,
    int nNumerator,
    int nDenominator
    )

{

    LONG Negate;
    union {
        LARGE_INTEGER Product;
        struct {
            ULONG Quotient;
            ULONG Remainder;
        };
    } u;

    //
    // Compute the size of the result.
    //

    Negate = nNumber ^ nNumerator ^ nDenominator;

    //
    // Get the absolute value of the operand values.
    //

    if (nNumber < 0) {
        nNumber = - nNumber;
    }

    if (nNumerator < 0) {
        nNumerator = - nNumerator;
    }

    if (nDenominator < 0) {
        nDenominator = - nDenominator;
    }

    //
    // Compute the 64-bit product of the multiplier and multiplicand
    // values and round.
    //

    u.Product.QuadPart =
        Int32x32To64(nNumber, nNumerator) + ((ULONG)nDenominator / 2);

    //
    // If there are any high order product bits, then the quotient has
    // overflowed.
    //

    if ((ULONG)nDenominator > u.Remainder) {

        //
        // Divide the 64-bit product by the 32-bit divisor forming a 32-bit
        // quotient and a 32-bit remainder.
        //

        u.Quotient = RtlEnlargedUnsignedDivide(*(PULARGE_INTEGER)&u.Product,
                                               (ULONG)nDenominator,
                                               &u.Remainder);

        //
        // Compute the final signed result.
        //

        if ((LONG)u.Quotient >= 0) {
            if (Negate >= 0) {
                return (LONG)u.Quotient;

            } else {
                return - (LONG)u.Quotient;
            }
        }
    }

    return - 1;
}

#endif

int
APIENTRY
lstrcmpA(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{
    int retval;

    retval = CompareStringA( GetThreadLocale(),
                             LOCALE_USE_CP_ACP,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringA( GetSystemDefaultLCID(),
                                 LOCALE_USE_CP_ACP,
                                 lpString1,
                                 -1,
                                 lpString2,
                                 -1 );
    }

    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            return strcmp(lpString1, lpString2);
        }
        else if (lpString1)
        {
            return (1);
        }
        else if (lpString2)
        {
            return (-1);
        }
        else
        {
            return (0);
        }
    }

    return (retval - 2);
}

int
APIENTRY
lstrcmpiA(
    LPCSTR lpString1,
    LPCSTR lpString2
    )
{
    int retval;

    retval = CompareStringA( GetThreadLocale(),
                             LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringA( GetSystemDefaultLCID(),
                                 LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                                 lpString1,
                                 -1,
                                 lpString2,
                                 -1 );
    }
    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            return ( _stricmp(lpString1, lpString2) );
        }
        else if (lpString1)
        {
            return (1);
        }
        else if (lpString2)
        {
            return (-1);
        }
        else
        {
            return (0);
        }
    }

    return (retval - 2);
}

LPSTR
APIENTRY
lstrcpyA(
    LPSTR lpString1,
    LPCSTR lpString2
    )
{
    __try {
        return strcpy(lpString1, lpString2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}


LPSTR
APIENTRY
lstrcpynA(
    LPSTR lpString1,
    LPCSTR lpString2,
    int iMaxLength
    )
{
    LPSTR src,dst;

    __try {
        src = (LPSTR)lpString2;
        dst = lpString1;

        if ( iMaxLength ) {
            while(iMaxLength && *src){
                *dst++ = *src++;
                iMaxLength--;
                }
            if ( iMaxLength ) {
                *dst = '\0';
                }
            else {
                dst--;
                *dst = '\0';
                }
            }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

   return lpString1;
}

LPSTR
APIENTRY
lstrcatA(
    LPSTR lpString1,
    LPCSTR lpString2
    )
{
    __try {
        return strcat(lpString1, lpString2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}

int
APIENTRY
lstrlenA(
    LPCSTR lpString
    )
{
    if (!lpString)
        return 0;
    __try {
        return strlen(lpString);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

int
APIENTRY
lstrcmpW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    int retval;

    retval = CompareStringW( GetThreadLocale(),
                             0,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringW( GetSystemDefaultLCID(),
                                 0,
                                 lpString1,
                                 -1,
                                 lpString2,
                                 -1 );
    }
    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            return ( wcscmp(lpString1, lpString2) );
        }
        else if (lpString1)
        {
            return (1);
        }
        else if (lpString2)
        {
            return (-1);
        }
        else
        {
            return (0);
        }
    }

    return (retval - 2);
}

int
APIENTRY
lstrcmpiW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    int retval;

    retval = CompareStringW( GetThreadLocale(),
                             NORM_IGNORECASE,
                             lpString1,
                             -1,
                             lpString2,
                             -1 );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringW( GetSystemDefaultLCID(),
                                 NORM_IGNORECASE,
                                 lpString1,
                                 -1,
                                 lpString2,
                                 -1 );
    }
    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            return ( _wcsicmp(lpString1, lpString2) );
        }
        else if (lpString1)
        {
            return (1);
        }
        else if (lpString2)
        {
            return (-1);
        }
        else
        {
            return (0);
        }
    }

    return (retval - 2);
}

LPWSTR
APIENTRY
lstrcpyW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    __try {
        return wcscpy(lpString1, lpString2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}

LPWSTR
APIENTRY
lstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR src,dst;

    __try {
        src = (LPWSTR)lpString2;
        dst = lpString1;

        if ( iMaxLength ) {
            while(iMaxLength && *src){
                *dst++ = *src++;
                iMaxLength--;
                }
            if ( iMaxLength ) {
                *dst = '\0';
                }
            else {
                dst--;
                *dst = '\0';
                }
            }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return lpString1;
}

LPWSTR
APIENTRY
lstrcatW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    __try {
        return wcscat(lpString1, lpString2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
}

int
APIENTRY
lstrlenW(
    LPCWSTR lpString
    )
{
    if (!lpString)
        return 0;
    __try {
        return wcslen(lpString);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}
