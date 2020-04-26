/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdffileobject.h

Abstract:

    This header containts the Windows Driver Framework file object
    DDIs.

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _WDFFILEOBJECT_H_
#define _WDFFILEOBJECT_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfFileObjectGetFileName
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
PUNICODE_STRING
(*PFN_WDFFILEOBJECTGETFILENAME)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT FileObject
    );

__drv_maxIRQL(PASSIVE_LEVEL)
PUNICODE_STRING
FORCEINLINE
WdfFileObjectGetFileName(
    __in
    WDFFILEOBJECT FileObject
    )
{
    return ((PFN_WDFFILEOBJECTGETFILENAME) WdfFunctions[WdfFileObjectGetFileNameTableIndex])(WdfDriverGlobals, FileObject);
}

//
// WDF Function: WdfFileObjectGetFlags
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
(*PFN_WDFFILEOBJECTGETFLAGS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT FileObject
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
WdfFileObjectGetFlags(
    __in
    WDFFILEOBJECT FileObject
    )
{
    return ((PFN_WDFFILEOBJECTGETFLAGS) WdfFunctions[WdfFileObjectGetFlagsTableIndex])(WdfDriverGlobals, FileObject);
}

//
// WDF Function: WdfFileObjectGetDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFFILEOBJECTGETDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT FileObject
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfFileObjectGetDevice(
    __in
    WDFFILEOBJECT FileObject
    )
{
    return ((PFN_WDFFILEOBJECTGETDEVICE) WdfFunctions[WdfFileObjectGetDeviceTableIndex])(WdfDriverGlobals, FileObject);
}

//
// WDF Function: WdfFileObjectWdmGetFileObject
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
(*PFN_WDFFILEOBJECTWDMGETFILEOBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT FileObject
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PFILE_OBJECT
FORCEINLINE
WdfFileObjectWdmGetFileObject(
    __in
    WDFFILEOBJECT FileObject
    )
{
    return ((PFN_WDFFILEOBJECTWDMGETFILEOBJECT) WdfFunctions[WdfFileObjectWdmGetFileObjectTableIndex])(WdfDriverGlobals, FileObject);
}




#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFFILEOBJECT_H_

