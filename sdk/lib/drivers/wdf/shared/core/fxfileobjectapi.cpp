/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectApi.cpp

Abstract:

    This modules implements the C API's for the FxFileObject.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"
#include "fxfileobject.hpp"

extern "C" {
// #include "FxFileObjectApi.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

__drv_maxIRQL(PASSIVE_LEVEL)
PUNICODE_STRING
STDCALL
WDFEXPORT(WdfFileObjectGetFileName)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the UNICODE_STRING for the FileName inside
    the WDM fileobject.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    PUNICODE_STRING (file name)

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxFileObject* pFO;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   FileObject,
                                   FX_TYPE_FILEOBJECT,
                                   (PVOID*)&pFO,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    if (pFO->GetWdmFileObject() != NULL) {
        return pFO->GetFileName();
    }
    else {
        return NULL;
    }
}


__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
STDCALL
WDFEXPORT(WdfFileObjectGetFlags)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the flags inside the WDM fileobject.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    ULONG (flags)

--*/

{
   DDI_ENTRY();

   FxFileObject* pFO;

   //
   // Validate the FileObject object handle, and get its FxFileObject*
   //
   FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                        FileObject,
                        FX_TYPE_FILEOBJECT,
                        (PVOID*)&pFO);

   if (pFO->GetWdmFileObject() != NULL) {
       return pFO->GetFlags();
   }
   else {
       return 0x0;
   }
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfFileObjectGetDevice)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the Device that the fileobject is associated with.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    WDFDEVICE

--*/

{
    DDI_ENTRY();

    FxFileObject* pFO;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                        FileObject,
                        FX_TYPE_FILEOBJECT,
                        (PVOID*)&pFO);

    return pFO->GetDevice()->GetHandle();
}

} // extern "C"
