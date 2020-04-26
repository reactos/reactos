/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfString.h

Abstract:

    This is the DDI for string handles.

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFSTRING_H_
#define _WDFSTRING_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfStringCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFSTRINGCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfStringCreate(
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    )
{
    return ((PFN_WDFSTRINGCREATE) WdfFunctions[WdfStringCreateTableIndex])(WdfDriverGlobals, UnicodeString, StringAttributes, String);
}

//
// WDF Function: WdfStringGetUnicodeString
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFSTRINGGETUNICODESTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfStringGetUnicodeString(
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    )
{
    ((PFN_WDFSTRINGGETUNICODESTRING) WdfFunctions[WdfStringGetUnicodeStringTableIndex])(WdfDriverGlobals, String, UnicodeString);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFSTRING_H_

