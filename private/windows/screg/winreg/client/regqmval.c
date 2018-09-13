/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regqmval.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    query multiple values APIs:
        - RegQueryMultipleValuesA
        - RegQueryMultipleValuesW

Author:

    John Vert (jvert) 15-Jun-1995

Revision History:

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"


WINADVAPI
LONG
APIENTRY
RegQueryMultipleValuesA (
    HKEY hKey,
    PVALENTA val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    )
/*++

Routine Description:

    The RegQueryMultipleValues function retrieves a list of
    data type/data pairs for a list of value names associated
    with an open registry key.

Parameters:

        hKey
                Identifies a currently open key or any of the pre-defined reserved handle values:
                HKEY_CLASSES_ROOT
                HEY_CURRENT_USER
                HKEY_LOCAL_MACHINE
                HKEY_USERS

        valList
                Points to an array of structures describing one or more value entries.  This
                contains the value names of the values to be queried.  Refer to Appendix A for a
                description of VALUE_ENTRY structure.

        num_vals
                Size of valList in bytes. If valListLength is not a multiple of the sizeof pvalue, the
                fractional extra space pointed to by valList is ignored.

        lpValueBuf
                The output buffer for returning value information (value names and value data). Data
                is DWORD aligned with pads inserted as necessary.

        ldwTotsize
                The total size of the output buffer pointed to by lpvalueBuf. On output ldwTotsize
                contains the number of bytes used including pads.  If lpValueBuf  was too short, then on
                output ldwTotsize will be the size needed, and caller should assume that lpValueBuf  was
                filled up to the size specified by ldwTotsize on input.

Return value:

    If the function succeeds, the return value is ERROR_SUCCESS; otherwise it is one
    of the error value which can be returned by RegQueryValueEx.  In addition, if
    either valList or lpValueBuf is too small then ERROR_INSUFFICIENT_BUFFER is returned
    If the function is unable to instantiate/access the provider of the
    dynamic key, it will return ERROR_CANTREAD.  If the total length of the
    requested data (valListLength + ldwTotSize) is more than the system limit of one
    megabytes, then the function returns ERROR_TRANSFER_TOO_LONG and only the first
    megabyte of data is returned.


--*/

{
    NTSTATUS Status;
    PRVALENT Values;
    PUNICODE_STRING Names;
    LONG Error;
    ULONG i;
    ULONG DataLength;
    ULONG InputLength;
    LPDWORD pTotalSize;
    DWORD TotalSize;
    ANSI_STRING AnsiString;
    LPSTR NewValueBuf = NULL;
    DWORD DataOffset;
    ULONG AnsiLength;
    HKEY    TempHandle = NULL;

    hKey = MapPredefinedHandle(hKey, &TempHandle);
    if (hKey == NULL) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Allocate an array of RVALENTs to describe the input value names
    //
    Values = RtlAllocateHeap(RtlProcessHeap(),0,num_vals * sizeof(RVALENT));
    if (Values == NULL) {
        Error = ERROR_OUTOFMEMORY;
        goto ExitCleanup;
    }
    ZeroMemory(Values, sizeof(RVALENT)*num_vals);

    //
    // Allocate an array of UNICODE_STRINGs to contain the input names
    //
    Names = RtlAllocateHeap(RtlProcessHeap(),0,num_vals * sizeof(UNICODE_STRING));
    if (Names == NULL) {
        Error = ERROR_OUTOFMEMORY;
        goto ExitCleanup;
    }
    ZeroMemory(Names, num_vals*sizeof(UNICODE_STRING));

    //
    // Convert the value names to UNICODE_STRINGs
    //
    for (i=0; i<num_vals; i++) {
        RtlInitAnsiString(&AnsiString, val_list[i].ve_valuename);
        Status = RtlAnsiStringToUnicodeString(&Names[i], &AnsiString, TRUE);
        if (!NT_SUCCESS(Status)) {
            Error =  RtlNtStatusToDosError( Status );
            goto Cleanup;
        }

        //
        //  Add the terminating NULL to the Length so that RPC transmits
        //  it.
        //
        Names[i].Length += sizeof( UNICODE_NULL );
        Values[i].rv_valuename = &Names[i];
    }

    //
    // Allocate a data buffer twice the size of the input buffer
    // so that any Unicode value data will fit before it is converted
    // to Ansi.
    //

    if ((ldwTotsize == NULL) || (*ldwTotsize == 0)) {
        TotalSize = 0;
    } else {
        TotalSize = *ldwTotsize * sizeof(WCHAR);
        NewValueBuf = RtlAllocateHeap(RtlProcessHeap(),0,TotalSize);
        if (NewValueBuf == NULL) {
            Error = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    pTotalSize = &TotalSize;

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if (IsLocalHandle(hKey)) {
        Error = (LONG)LocalBaseRegQueryMultipleValues(hKey,
                                                      Values,
                                                      num_vals,
                                                      NewValueBuf,
                                                      pTotalSize);
    } else {
        DWORD dwVersion;
        Error = (LONG)BaseRegQueryMultipleValues(DereferenceRemoteHandle( hKey ),
                                                 Values,
                                                 num_vals,
                                                 NewValueBuf,
                                                 pTotalSize);
        if ((Error == ERROR_SUCCESS) &&
            (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion))) {
            //
            // Win95's RegQueryMultipleValues doesn't return Unicode
            // value data, so do not try and convert it back to Ansi.
            //
            for (i=0; i<num_vals; i++) {
                val_list[i].ve_valuelen = Values[i].rv_valuelen;
                val_list[i].ve_type = Values[i].rv_type;
                val_list[i].ve_valueptr = (DWORD_PTR)(lpValueBuf + Values[i].rv_valueptr);
            }
            CopyMemory(lpValueBuf,NewValueBuf,TotalSize);
            if (ldwTotsize != NULL) {
                *ldwTotsize = TotalSize;
            }
            goto Cleanup;
        }
    }
    if (Error == ERROR_SUCCESS) {
        //
        // Convert results back.
        //
        DataOffset = 0;
        for (i=0; i < num_vals; i++) {
            val_list[i].ve_valuelen = Values[i].rv_valuelen;
            val_list[i].ve_type = Values[i].rv_type;
            val_list[i].ve_valueptr = (DWORD_PTR)(lpValueBuf + DataOffset);
            if ((val_list[i].ve_type == REG_SZ) ||
                (val_list[i].ve_type == REG_EXPAND_SZ) ||
                (val_list[i].ve_type == REG_MULTI_SZ)) {

                Status = RtlUnicodeToMultiByteN(lpValueBuf + DataOffset,
                                                Values[i].rv_valuelen/sizeof(WCHAR),
                                                &AnsiLength,
                                                (PWCH)(NewValueBuf + Values[i].rv_valueptr),
                                                Values[i].rv_valuelen);
                if (!NT_SUCCESS(Status)) {
                    Error =  RtlNtStatusToDosError( Status );
                }
                val_list[i].ve_valuelen = AnsiLength;
                DataOffset += AnsiLength;
            } else {
                CopyMemory(lpValueBuf + DataOffset,
                           NewValueBuf + Values[i].rv_valueptr,
                           Values[i].rv_valuelen);
                DataOffset += Values[i].rv_valuelen;
            }
            //
            // Round DataOffset up to dword boundary.
            //
            DataOffset = (DataOffset + sizeof(DWORD) - 1) & ~(sizeof(DWORD)-1);
        }
        if (ldwTotsize != NULL) {
            *ldwTotsize = DataOffset;
        }
    } else if (Error == ERROR_MORE_DATA) {
        //
        // We need to thunk the Unicode required bytes back to Ansi. But
        // there is not really any way to do this without having the data
        // available. So just return the required bytes for the Unicode
        // data, as this will always be enough.
        //
        if (ldwTotsize != NULL) {
            *ldwTotsize = *pTotalSize;
        }
    }

Cleanup:
    if (NewValueBuf != NULL) {
        RtlFreeHeap(RtlProcessHeap(),0,NewValueBuf);
    }
    for (i=0; i<num_vals; i++) {
        if (Names[i].Buffer != NULL) {
            RtlFreeUnicodeString(&Names[i]);
        }
    }
    RtlFreeHeap(RtlProcessHeap(),0,Values);
    RtlFreeHeap(RtlProcessHeap(),0,Names);

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

WINADVAPI
LONG
APIENTRY
RegQueryMultipleValuesW (
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    )
/*++

Routine Description:

    The RegQueryMultipleValues function retrieves a list of
    data type/data pairs for a list of value names associated
    with an open registry key.

Parameters:

        hKey
                Identifies a currently open key or any of the pre-defined reserved handle values:
                HKEY_CLASSES_ROOT
                HEY_CURRENT_USER
                HKEY_LOCAL_MACHINE
                HKEY_USERS

        valList
                Points to an array of structures describing one or more value entries.  This
                contains the value names of the values to be queried.  Refer to Appendix A for a
                description of VALUE_ENTRY structure.

        num_vals
                Size of valList in bytes. If valListLength is not a multiple of the sizeof pvalue, the
                fractional extra space pointed to by valList is ignored.

        lpValueBuf
                The output buffer for returning value information (value names and value data). Data
                is DWORD aligned with pads inserted as necessary.

        ldwTotsize
                The total size of the output buffer pointed to by lpValueBuf. On output ldwTotsize
                contains the number of bytes used including pads.  If lpValueBuf  was too short, then on
                output ldwTotsize will be the size needed, and caller should assume that lpValueBuf  was
                filled up to the size specified by ldwTotsize on input.

Return value:

    If the function succeeds, the return value is ERROR_SUCCESS; otherwise it is one
    of the error value which can be returned by RegQueryValueEx.  In addition, if
    either valList or lpValueBuf is too small then ERROR_INSUFFICIENT_BUFFER is returned
    If the function is unable to instantiate/access the provider of the
    dynamic key, it will return ERROR_CANTREAD.  If the total length of the
    requested data (valListLength + ldwTotSize) is more than the system limit of one
    megabytes, then the function returns ERROR_TRANSFER_TOO_LONG and only the first
    megabyte of data is returned.


--*/

{
    NTSTATUS Status;
    PRVALENT Values;
    PUNICODE_STRING Names;
    LONG Error;
    ULONG i;
    ULONG DataLength;
    ULONG InputLength;
    LPDWORD pTotalSize;
    DWORD TotalSize;
    DWORD StringLength;
    HKEY    TempHandle = NULL;

    hKey = MapPredefinedHandle(hKey, &TempHandle);
    if (hKey == NULL) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Allocate an array of RVALENTs to describe the input value names
    //
    Values = RtlAllocateHeap(RtlProcessHeap(),0,num_vals * sizeof(RVALENT));
    if (Values == NULL) {
        Error = ERROR_OUTOFMEMORY;
        goto ExitCleanup;
    }
    ZeroMemory(Values, sizeof(RVALENT)*num_vals);

    //
    // Allocate an array of UNICODE_STRINGs to contain the input names
    //
    Names = RtlAllocateHeap(RtlProcessHeap(),0,num_vals * sizeof(UNICODE_STRING));
    if (Names == NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, Values);
        Error = ERROR_OUTOFMEMORY;
        goto ExitCleanup;
    }
    ZeroMemory(Names, num_vals*sizeof(UNICODE_STRING));

    //
    // Copy and convert the value names to UNICODE_STRINGs
    // Note that we have to copy the value names because RPC tromps
    // on them.
    //
    for (i=0; i<num_vals; i++) {

        StringLength = wcslen(val_list[i].ve_valuename)*sizeof(WCHAR);
        Names[i].Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, StringLength + sizeof(UNICODE_NULL));
        if (Names[i].Buffer == NULL) {
            goto error_exit;
        }
        Names[i].Length = Names[i].MaximumLength = (USHORT)StringLength + sizeof(UNICODE_NULL);
        CopyMemory(Names[i].Buffer, val_list[i].ve_valuename, StringLength + sizeof(UNICODE_NULL));

        Values[i].rv_valuename = &Names[i];
    }

    if (ldwTotsize == NULL) {
        TotalSize = 0;
        pTotalSize = &TotalSize;
    } else {
        pTotalSize = ldwTotsize;
    }

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if (IsLocalHandle(hKey)) {
        Error = (LONG)LocalBaseRegQueryMultipleValues(hKey,
                                                      Values,
                                                      num_vals,
                                                      (LPSTR)lpValueBuf,
                                                      pTotalSize);
    } else {
        DWORD dwVersion;
        if (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion)) {
            //
            // We cannot support RegQueryMultipleValuesW to Win95 servers
            // since they do not return Unicode value data.
            //
            Error = ERROR_CALL_NOT_IMPLEMENTED;
        } else {
            Error = (LONG)BaseRegQueryMultipleValues(DereferenceRemoteHandle( hKey ),
                                                     Values,
                                                     num_vals,
                                                     (LPSTR)lpValueBuf,
                                                     pTotalSize);
        }
    }
    if (Error == ERROR_SUCCESS) {
        //
        // Convert results back.
        //
        for (i=0; i < num_vals; i++) {
            val_list[i].ve_valuelen = Values[i].rv_valuelen;
            val_list[i].ve_valueptr = (DWORD_PTR)((LPCSTR)lpValueBuf + Values[i].rv_valueptr);
            val_list[i].ve_type = Values[i].rv_type;
        }
    }

error_exit:
    for (i=0; i < num_vals; i++) {
        if (Names[i].Buffer != NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, Names[i].Buffer);
        }
    }

    RtlFreeHeap(RtlProcessHeap(),0,Values);
    RtlFreeHeap(RtlProcessHeap(),0,Names);

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
