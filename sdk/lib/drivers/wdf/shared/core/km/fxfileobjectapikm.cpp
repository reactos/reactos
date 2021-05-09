/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectApiUm.cpp

Abstract:

    This modules implements the C API's for the FxFileObject.

Author:

Environment:

    Kernel Mode mode only

Revision History:


--*/

#include "coreprivshared.hpp"
#include "fxfileobject.hpp"

extern "C" {
// #include "FxFileObjectApiKm.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
PFILE_OBJECT
STDCALL
WDFEXPORT(WdfFileObjectWdmGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT  FileObject
    )
/*++

Routine Description:

    This functions returns the corresponding WDM fileobject. If the device is opened
    by a kernel-mode componenet by sending a IRP_MJ_CREATE irp
    directly without a fileobject, this call can return a NULL pointer.

    Creating a WDFFILEOBJECT without an underlying WDM fileobject
    is done only for 'exclusive' devices.

    Serenum sends such a create-irp to the serial driver.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:


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

   return pFO->GetWdmFileObject();
}

} // extern "C"
